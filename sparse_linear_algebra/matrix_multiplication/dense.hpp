#pragma once

#include "Eigen/Dense"
#include "mpc_utils/boost_serialization/eigen.hpp"
#include "mpc_utils/comm_channel.hpp"
#include "sparse_linear_algebra/matrix_multiplication/offline/triple_provider.hpp"
#include "sparse_linear_algebra/oblivious_map/oblivious_map.hpp"

/**
 * error_info structs for reporting input dimension in exceptions
 */
typedef boost::error_info<struct tag_TRIPLE_L, std::tuple<int, int, int>>
    error_triple_dimensions;
typedef boost::error_info<struct tag_DIM_A_SIZE1, size_t> error_a_size1;
typedef boost::error_info<struct tag_DIM_A_SIZE2, size_t> error_a_size2;
typedef boost::error_info<struct tag_DIM_B_SIZE1, size_t> error_b_size1;
typedef boost::error_info<struct tag_DIM_B_SIZE2, size_t> error_b_size2;
typedef boost::error_info<struct tag_K_A, size_t> error_k_A;
typedef boost::error_info<struct tag_K_B, size_t> error_k_B;
typedef boost::error_info<struct tag_CHUNK_SIZE, size_t> error_chunk_size;

template <
    typename Derived_A, typename Derived_B,
    typename T = typename Derived_A::Scalar, bool is_shared,
    typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value,
                            int>::type = 0>
Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
matrix_multiplication_dense(
    const Eigen::EigenBase<Derived_A> &A_in,
    const Eigen::EigenBase<Derived_B> &B_in, comm_channel &channel, int role,
    sparse_linear_algebra::matrix_multiplication::offline::TripleProvider<
        T, is_shared> &triples,
    ssize_t chunk_size_in = -1) {
  using matrix_result = Eigen::Matrix<T, Derived_A::RowsAtCompileTime,
                                      Derived_B::ColsAtCompileTime>;
  const Derived_A &A = A_in.derived();
  const Derived_B &B = B_in.derived();
  // A : l x m, B: m x n, C: l x n
  size_t l = A.rows(), m = A.cols(), n = B.cols();
  size_t chunk_size = chunk_size_in;
  if (chunk_size_in == -1) {
    chunk_size = l;
  }

  try {
    // check if argument sizes are valid
    if (m != B.rows()) {
      BOOST_THROW_EXCEPTION(std::invalid_argument("Matrix sizes do not match"));
    }
    if (std::tie(chunk_size, m, n) != triples.dimensions()) {
      BOOST_THROW_EXCEPTION(
          boost::enable_error_info(std::invalid_argument(
              "Triple dimensions do not match matrix dimensions"))
          << error_triple_dimensions(triples.dimensions()));
    }

    using matrix_triple =
        sparse_linear_algebra::matrix_multiplication::offline::Matrix<T>;
    std::vector<std::function<matrix_triple()>> compute_chunks;
    for (size_t i = 0; i * chunk_size < l; i++) {
      compute_chunks.push_back([&triples, &channel, &B, &A, chunk_size, l, m, n,
                                role, i]() -> matrix_triple {
        Eigen::Matrix<T, Eigen::Dynamic, Derived_A::ColsAtCompileTime> chunk_A(
            chunk_size, m);
        chunk_A.setZero();
        if ((i + 1) * chunk_size > l) {
          chunk_A += A.bottomRows(l - i * chunk_size);
        } else {
          chunk_A = A.middleRows(i * chunk_size, chunk_size);
        }
        // get a multiplication Triple;
        matrix_triple U, V, Z;
        std::tie(U, V, Z) = triples.GetTriple();

        // role 0 sends A - U and receives B - V simultaneously
        // then compute share of the result
        matrix_triple E = (is_shared || role == 0) * (chunk_A - U),
                      E2(chunk_size, m);
        matrix_triple F = (is_shared || role == 1) * (B - V), F2(m, n);
        if (role == 0) {
          channel.send_recv(E, F2);
          if (is_shared) {
            channel.send_recv(F, E2);
            E += E2;
          }
          F += F2;
          return (E * V) + (U * F) + Z;
        } else {
          channel.send_recv(F, E2);
          if (is_shared) {
            channel.send_recv(E, F2);
            F += F2;
          }
          E += E2;
          return (E * F) + (E * V) + (U * F) + Z;
        }
      });
    }
    matrix_result result = matrix_result::Zero(l, n);
    // TODO: threading
    for (size_t i = 0; i * chunk_size < l; i++) {
      if ((i + 1) * chunk_size > l) {
        result.bottomRows(l - i * chunk_size) =
            compute_chunks[i]().topRows(l - i * chunk_size);
      } else {
        result.middleRows(i * chunk_size, chunk_size) = compute_chunks[i]();
      }
    }
    return result;
  } catch (boost::exception &e) {
    e << error_a_size1(A.rows()) << error_a_size2(A.cols())
      << error_b_size1(B.rows()) << error_b_size2(B.cols());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
