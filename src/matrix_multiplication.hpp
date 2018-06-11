#pragma once
#include "mpc-utils/comm_channel.hpp"
#include "util/blocking_queue.hpp"
#include "util/randomize_matrix.hpp"
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/optional.hpp>

/**
 * error_info structs for reporting input dimension in exceptions
 */
typedef boost::error_info<struct tag_TRIPLE_L,size_t> error_triple_l;
typedef boost::error_info<struct tag_TRIPLE_M,size_t> error_triple_m;
typedef boost::error_info<struct tag_TRIPLE_N,size_t> error_triple_n;
typedef boost::error_info<struct tag_DIM_A_SIZE1,size_t> error_a_size1;
typedef boost::error_info<struct tag_DIM_A_SIZE2,size_t> error_a_size2;
typedef boost::error_info<struct tag_DIM_B_SIZE1,size_t> error_b_size1;
typedef boost::error_info<struct tag_DIM_B_SIZE2,size_t> error_b_size2;
typedef boost::error_info<struct tag_K_A,size_t> error_k_A;
typedef boost::error_info<struct tag_K_B,size_t> error_k_B;
typedef boost::error_info<struct tag_CHUNK_SIZE,size_t> error_chunk_size;

template<typename T, bool is_shared = false>
class triple_provider {
protected:
  size_t l, m, n;
  int role;

public:
  using triple = std::tuple<boost::numeric::ublas::matrix<T>,
                   boost::numeric::ublas::matrix<T>,
                   boost::numeric::ublas::matrix<T> >;

  triple_provider(size_t l, size_t m, size_t n, int role):
    l(l), m(m), n(n), role(role) {};

  size_t get_l() { return l; }
  size_t get_m() { return m; }
  size_t get_n() { return n; }

  virtual triple get() = 0;
};

template<typename T, bool is_shared = false>
class fake_triple_provider : public virtual triple_provider<T, is_shared> {
using triple = typename triple_provider<T, is_shared>::triple;
private:
  blocking_queue<triple> triples;
  int seed = 34567; // seed random number generator deterministically
  std::mt19937 r;

  triple compute_fake_triple() {
      using namespace boost::numeric::ublas;
      size_t l = this->l, m = this->m, n = this->n;
      int role = this->role;

      std::uniform_int_distribution<T> dist;
      matrix<T> U(l, m), U_mask = is_shared ? matrix<T>(l, m) : zero_matrix<T>(l, m);
      matrix<T> V(m, n), V_mask = is_shared ? matrix<T>(m, n) : zero_matrix<T>(m, n);
      matrix<T> Z_mask(l, n);

      randomize_matrix(r, U);
      randomize_matrix(r, V);
      randomize_matrix(r, Z_mask);
      if(is_shared) {
        randomize_matrix(r, U_mask);
        randomize_matrix(r, V_mask);
      }

      if(role == 0) {
          return std::make_tuple(U - U_mask, V_mask, Z_mask);
      } else {
          return std::make_tuple(U_mask, V - V_mask, prod(U, V) - Z_mask);
      }
  }

public:
  // the `cap` argument allows to use a bounded queue for storing triples,
  fake_triple_provider(size_t l, size_t m, size_t n, int role, ssize_t cap = -1):
    triple_provider<T, is_shared>(l, m, n, role), triples(cap), r(seed) {};
  // blocks if capacity is bounded
  void precompute(size_t num) {
    for(size_t i = 0; i < num; i++) {
      triples.push(this->compute_fake_triple());
    }
  }
  // thread-safe!
  triple get() {
    return triples.pop();
  }
};


template<class MATRIX_A, class MATRIX_B,
  typename T = typename MATRIX_A::value_type, bool is_shared,
  typename std::enable_if<std::is_same<T, typename MATRIX_B::value_type>::value, int>::type = 0>
boost::numeric::ublas::matrix<T> matrix_multiplication(
    const boost::numeric::ublas::matrix_expression<MATRIX_A>& A_in,
    const boost::numeric::ublas::matrix_expression<MATRIX_B>& B_in,
    comm_channel& channel, int role,
    triple_provider<T, is_shared>& triples,
    ssize_t chunk_size_in = -1
) {
  using namespace boost::numeric::ublas;
  const MATRIX_A& A = A_in();
  const MATRIX_B& B = B_in();
  // A : l x m, B: m x n, C: l x n
  size_t l = A.size1(), m = A.size2(), n = B.size2();
  size_t chunk_size = chunk_size_in;
  if(chunk_size_in == -1) {
    chunk_size = l;
  }

  try {
    // check if argument sizes are valid
    if (m != B.size1()) {
        BOOST_THROW_EXCEPTION(std::invalid_argument("Matrix sizes do not match"));
    }
    if(chunk_size != triples.get_l() || triples.get_m() != m || triples.get_n() != n) {
        BOOST_THROW_EXCEPTION(boost::enable_error_info(std::invalid_argument(
            "Triple dimensions do not match matrix dimensions"))
            << error_triple_l(triples.get_l()) << error_triple_m(triples.get_m())
            << error_triple_n(triples.get_n()));
    }
    if(l % chunk_size) { //TODO: allow for different chunk sizes
        BOOST_THROW_EXCEPTION(boost::enable_error_info(std::invalid_argument(
            "`A_in.size1()` must be divisible by `chunk_size`")));
    }

    std::vector<std::function<matrix<T>()>> compute_chunks;
    for(size_t i = 0; i*chunk_size < l; i++) {
      compute_chunks.push_back([&triples, &channel, &B, &A, chunk_size, m, n, role, i]() -> matrix<T> {
        auto chunk_A = subrange(A, i*chunk_size, (i+1)*chunk_size, 0, A.size2());
        // get a multiplication triple
        matrix<T> U, V, Z;
        std::tie(U, V, Z) = triples.get();

        // role 0 sends A - U and receives B - V simultaneously
        // then compute share of the result
        matrix<T, row_major> E = (is_shared || role == 0) * (chunk_A - U), E2(chunk_size, m);
        matrix<T, row_major> F = (is_shared || role == 1) * (B - V), F2(m, n);
        if(role == 0) {
            channel.send_recv(E, F2);
            if(is_shared) {
                channel.send_recv(F, E2);
                E += E2;
            }
            F += F2;
            return prod(E, V) + prod(U, F) + Z;
        } else {
            channel.send_recv(F, E2);
            if(is_shared) {
                channel.send_recv(E, F2);
                F += F2;
            }
            E += E2;
            return prod(E, F) + prod(E, V) + prod(U, F) + Z;
        }
      });
    }
    matrix<T> result = zero_matrix<T>(l, n);
    // TODO: threading
    for(size_t i = 0; i*chunk_size < l; i++) {
      subrange(result, i*chunk_size, (i+1)*chunk_size, 0, result.size2()) = compute_chunks[i]();
    }
    return result;
  } catch(boost::exception& e) {
    e << error_a_size1(A.size1()) << error_a_size2(A.size2())
      << error_b_size1(B.size1()) << error_b_size2(B.size2());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
