#pragma once

#include <Eigen/Sparse>
#include <boost/range/algorithm.hpp>
#include <boost/range/counting_range.hpp>
#include <unordered_map>
#include <unordered_set>
#include "sparse_common.hpp"
#include "sparse_linear_algebra/matrix_multiplication/dense.hpp"
#include "sparse_linear_algebra/oblivious_map/oblivious_map.hpp"
#include "sparse_linear_algebra/util/time.h"
#include "sparse_linear_algebra/zero_sharing/zero_sharing.hpp"
extern "C" {
#include "bcrandom.h"
}

template <
    typename Derived_A, typename Derived_B,
    typename T = typename Derived_A::Scalar,
    typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value,
                            int>::type = 0>
Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
matrix_multiplication_rows_dense(
    const Eigen::SparseMatrixBase<Derived_A>& A_in,
    const Eigen::MatrixBase<Derived_B>& B_in, comm_channel& channel, int role,
    sparse_linear_algebra::matrix_multiplication::offline::TripleProvider<
        T, false>& triples,
    ssize_t chunk_size_in = -1,
    ssize_t k_A = -1,  // saves a communication round if set
    mpc_utils::Benchmarker* benchmarker = nullptr) {
  try {
    std::vector<size_t> inner_indices;
    Eigen::SparseMatrix<T, Eigen::ColMajor> A;
    auto& B = B_in.derived();
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
        ret, ret_dense;
    // compute own indices and exchange k values if not given as arguments
    if (role == 0) {
      A = A_in.derived();
      inner_indices = compute_inner_indices(A);
      if (k_A == -1) {
        k_A = inner_indices.size();
        channel.send(k_A);
        channel.flush();
      }
    } else {
      if (k_A == -1) {
        channel.recv(k_A);
      }
      A.resize(k_A, A_in.cols());
      A.setZero();
    }

    mpc_utils::Benchmarker::time_point start;
    if (benchmarker != nullptr) {
      start = benchmarker->StartTimer();
    }

    // extract nonzero rows
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_A::ColsAtCompileTime>
        A_dense;
    if (role == 0) {
      Eigen::SparseMatrix<T, Eigen::RowMajor> A_rows = A;
      A_dense.resize(k_A, A.cols());
      A_dense.setZero();
      for (size_t i = 0; i < k_A; i++) {
        A_dense.row(i) = A_rows.row(inner_indices[i]);
      }
    }

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("reordering_time", start);
      start = benchmarker->StartTimer();
    }

    // dense multiplication
    if (role == 0) {
      ret_dense = matrix_multiplication_dense(A_dense, B, channel, role,
                                              triples, chunk_size_in);
    } else {
      ret_dense = matrix_multiplication_dense(A, B, channel, role, triples,
                                              chunk_size_in);
    }

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("dense_time", start);
      start = benchmarker->StartTimer();
    }

    // use zero-sharing protocol to share the result back to dimension A.rows()
    ret.resize(A_in.rows(), B_in.cols());
    ret.setZero();
    for (size_t col = 0; col < B_in.cols(); col++) {
      std::vector<T> current_col_dense(ret_dense.rows());
      std::vector<T> current_col;
      for (size_t row = 0; row < ret_dense.rows(); row++) {
        current_col_dense[row] = ret_dense(row, col);
      }
      if (role == 0) {
        current_col = zero_sharing_server(current_col_dense, inner_indices,
                                          A_in.rows(), channel, benchmarker);
      } else {
        current_col = zero_sharing_client(current_col_dense, A_in.rows(),
                                          channel, benchmarker);
      }
      for (size_t row = 0; row < ret.rows(); row++) {
        ret(row, col) = current_col[row];
      }
    }

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("zero_sharing_time", start);
      start = benchmarker->StartTimer();
    }

    return ret;

  } catch (boost::exception& e) {
    e << error_a_size1(A_in.rows()) << error_a_size2(A_in.cols())
      << error_b_size1(B_in.rows()) << error_b_size2(B_in.cols());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
