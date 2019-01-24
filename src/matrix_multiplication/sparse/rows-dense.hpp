#include <unordered_map>
#include <unordered_set>
#include <boost/range/algorithm.hpp>
#include <boost/range/counting_range.hpp>
#include <Eigen/Sparse>
#include "../dense.hpp"
#include "sparse_common.hpp"
#include "src/pir_protocol.hpp"
#include "src/util/time.h"
#include "src/zero_sharing/zero_sharing.hpp"
extern "C" {
  #include <bcrandom.h>
}


template<typename Derived_A, typename Derived_B,
  typename T = typename Derived_A::Scalar,
  typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value, int>::type = 0>
Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
matrix_multiplication_rows_dense(
    const Eigen::SparseMatrixBase<Derived_A>& A_in,
    const Eigen::MatrixBase<Derived_B>& B_in,
    comm_channel& channel, int role,
    triple_provider<T, false>& triples,
    ssize_t chunk_size_in = -1,
    ssize_t k_A = -1, // saves a communication round if set
    bool print_times = false
) {
  try {
    std::vector<size_t> inner_indices;
    Eigen::SparseMatrix<T, Eigen::ColMajor> A;
    auto& B = B_in.derived();
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
      ret, ret_dense;
    // compute own indices and exchange k values if not given as arguments
    if(role == 0) {
      A = A_in.derived();
      inner_indices = compute_inner_indices(A);
      if(k_A == -1) {
        k_A = inner_indices.size();
        channel.send(k_A);
        channel.flush();
      }
    } else {
      if(k_A == -1) {
        channel.recv(k_A);
      }
      A.resize(k_A, A_in.cols());
      A.setZero();
    }
    double start = timestamp();

    // extract nonzero rows
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_A::ColsAtCompileTime>
      A_dense;
    if(role == 0) {
      Eigen::SparseMatrix<T, Eigen::RowMajor> A_rows = A;
      A_dense.resize(k_A, A.cols());
      A_dense.setZero();
      for(size_t i = 0; i < k_A; i++) {
        A_dense.row(i) = A_rows.row(inner_indices[i]);
      }
    }
    if(print_times) {
      double end = timestamp();
      std::cout << "reordering_time: " << end - start << " s\n";
      start = end;
    }

    // dense multiplication
    if(role == 0) {
      ret_dense = matrix_multiplication(A_dense, B, channel, role, triples, chunk_size_in);
    } else {
      ret_dense = matrix_multiplication(A, B, channel, role, triples, chunk_size_in);
    }
    if(print_times) {
      double end = timestamp();
      std::cout << "dense_time: " << end - start << " s\n";
      start = end;
    }

    // use zero-sharing protocol to share the result back to dimension A.rows()
    ret.resize(A_in.rows(), B_in.cols());
    ret.setZero();
    for(size_t col = 0; col < B_in.cols(); col++) {
      std::vector<T> current_col_dense(ret_dense.rows());
      std::vector<T> current_col;
      for(size_t row = 0; row < ret_dense.rows(); row++) {
        current_col_dense[row] = ret_dense(row, col);
      }
      if(role == 0) {
        current_col = zero_sharing_server(current_col_dense, inner_indices,
          A_in.rows(), channel);
      } else {
        current_col = zero_sharing_client(current_col_dense, A_in.rows(),
          channel);
      }
      for(size_t row = 0; row < ret.rows(); row++) {
        ret(row, col) = current_col[row];
      }
    }
    if(print_times) {
      double end = timestamp();
      std::cout << "zero_sharing_time: " << end - start << " s\n";
      start = end;
    }

    return ret;

  } catch(boost::exception& e) {
    e << error_a_size1(A_in.rows()) << error_a_size2(A_in.cols())
      << error_b_size1(B_in.rows()) << error_b_size2(B_in.cols());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
