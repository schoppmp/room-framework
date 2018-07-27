#include <unordered_map>
#include <unordered_set>
#include <boost/range/algorithm.hpp>
#include <boost/range/counting_range.hpp>
#include <Eigen/Sparse>
#include "../dense.hpp"
#include "sparse_common.hpp"
#include "pir_protocol.hpp"
#include "util/time.h"
extern "C" {
  #include <bcrandom.h>
}


template<typename Derived_A, typename Derived_B,
  typename T = typename Derived_A::Scalar,
  typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value, int>::type = 0>
Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
matrix_multiplication_cols_dense(
    const Eigen::SparseMatrixBase<Derived_A>& A_in,
    const Eigen::MatrixBase<Derived_B>& B_in,
    pir_protocol<size_t, T>& prot,
    comm_channel& channel, int role,
    triple_provider<T, true>& triples,
    ssize_t chunk_size_in = -1,
    ssize_t k_A = -1, // saves a communication round if set
    bool print_times = false
) {
  try {
    std::vector<size_t> inner_indices;
    Eigen::SparseMatrix<T, Eigen::RowMajor> A;
    Eigen::Matrix<T, Derived_B::RowsAtCompileTime, Derived_B::ColsAtCompileTime> B;
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime> ret;
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
      B = B_in.derived();
      if(k_A == -1) {
        channel.recv(k_A);
      }
      A.resize(A_in.rows(), k_A);
      A.setZero();
    }
    double start = timestamp();

    // get additive shares of B at inner_indices
    // TODO: extend ROOM to multiple values, do whole matrix B at once
    size_t num_cols_B = B_in.derived().cols();
    Eigen::Matrix<T, Derived_B::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
      B_shared(k_A, num_cols_B);
    B_shared.setZero();
    for(size_t col = 0; col < num_cols_B; col++) {
      std::vector<T> result(k_A);
      if(role == 0) {
        prot.run_client(inner_indices, result, true);
      } else {
        // set our share
        auto rng = newBCipherRandomGen();
        randomizeBuffer(rng, (char*) result.data(), result.size() * sizeof(T));
        releaseBCipherRandomGen(rng);
        // get current column
        std::vector<T> values(B.rows());
        for(size_t row = 0; row < B.rows(); row++) {
          values[row] = B(row, col);
        }
        prot.run_server(boost::counting_range(size_t(0), size_t(B.rows())),
          values, result, true);
      }
      for(size_t row = 0; row < k_A; row++) {
        B_shared(row, col) = result[row];
      }
    }
    if(print_times) {
      double end = timestamp();
      std::cout << "room_time: " << end - start << " s\n";
      start = end;
    }

    // extract nonzero rows
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_A::ColsAtCompileTime>
      A_dense;
    if(role == 0) {
      Eigen::SparseMatrix<T, Eigen::ColMajor> A_cols = A;
      A_dense.resize(A.rows(), k_A);
      A_dense.setZero();
      for(size_t i = 0; i < k_A; i++) {
        A_dense.col(i) = A_cols.col(inner_indices[i]);
      }
    }
    if(print_times) {
      double end = timestamp();
      std::cout << "reordering_time: " << end - start << " s\n";
      start = end;
    }

    // dense multiplication
    if(role == 0) {
      ret = matrix_multiplication(A_dense, B_shared, channel, role, triples, chunk_size_in);
    } else {
      ret = matrix_multiplication(A, B_shared, channel, role, triples, chunk_size_in);
    }
    if(print_times) {
      double end = timestamp();
      std::cout << "dense_time: " << end - start << " s\n";
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
