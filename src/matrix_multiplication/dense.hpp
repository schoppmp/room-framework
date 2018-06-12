#pragma once
#include <boost/optional.hpp>

#include "mpc-utils/comm_channel.hpp"
#include "mpc-utils/boost_serialization/eigen.hpp"
#include "util/blocking_queue.hpp"
#include "util/randomize_matrix.hpp"
#include "pir_protocol.hpp"

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
  using matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  using triple = std::tuple<matrix, matrix, matrix>;

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
      using matrix = typename triple_provider<T, is_shared>::matrix;
      size_t l = this->l, m = this->m, n = this->n;
      int role = this->role;

      std::uniform_int_distribution<T> dist;
      matrix U(l, m), U_mask = matrix::Zero(l, m);
      matrix V(m, n), V_mask = matrix::Zero(m, n);
      matrix Z_mask(l, n);

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
          return std::make_tuple(U_mask, V - V_mask, U * V - Z_mask);
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


template<class Derived_A, class Derived_B,
  typename T = typename Derived_A::Scalar, bool is_shared,
  typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value, int>::type = 0>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrix_multiplication(
    const Eigen::MatrixBase<Derived_A>& A_in,
    const Eigen::MatrixBase<Derived_B>& B_in,
    comm_channel& channel, int role,
    triple_provider<T, is_shared>& triples,
    ssize_t chunk_size_in = -1
) {
  using matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  const Derived_A& A = A_in.derived();
  const Derived_B& B = B_in.derived();
  // A : l x m, B: m x n, C: l x n
  size_t l = A.rows(), m = A.cols(), n = B.cols();
  size_t chunk_size = chunk_size_in;
  if(chunk_size_in == -1) {
    chunk_size = l;
  }

  try {
    // check if argument sizes are valid
    if (m != B.rows()) {
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
            "`A_in.rows()` must be divisible by `chunk_size`")));
    }

    std::vector<std::function<matrix()>> compute_chunks;
    for(size_t i = 0; i*chunk_size < l; i++) {
      compute_chunks.push_back([&triples, &channel, &B, &A, chunk_size, m, n, role, i]() -> matrix {
        auto chunk_A = A.block(i * chunk_size, 0, chunk_size, A.cols());
        // get a multiplication triple; TODO: threaded triple provider
        matrix U, V, Z;
        std::tie(U, V, Z) = triples.get();

        // role 0 sends A - U and receives B - V simultaneously
        // then compute share of the result
        matrix E = (is_shared || role == 0) * (chunk_A - U), E2(chunk_size, m);
        matrix F = (is_shared || role == 1) * (B - V), F2(m, n);
        if(role == 0) {
            channel.send_recv(E, F2);
            if(is_shared) {
                channel.send_recv(F, E2);
                E += E2;
            }
            F += F2;
            return (E * V) + (U * F) + Z;
        } else {
            channel.send_recv(F, E2);
            if(is_shared) {
                channel.send_recv(E, F2);
                F += F2;
            }
            E += E2;
            return (E * F) + (E * V) + (U * F) + Z;
        }
      });
    }
    matrix result = matrix::Zero(l, n);
    // TODO: threading
    for(size_t i = 0; i*chunk_size < l; i++) {
      result.block(i*chunk_size, 0, chunk_size, result.cols()) = compute_chunks[i]();
    }
    return result;
  } catch(boost::exception& e) {
    e << error_a_size1(A.rows()) << error_a_size2(A.cols())
      << error_b_size1(B.rows()) << error_b_size2(B.cols());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
