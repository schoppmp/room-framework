#pragma once

#include <numeric>
#include <random>
#include <sstream>
#include <unordered_map>
#include "Eigen/Sparse"
#include "boost/range/algorithm.hpp"
#include "sparse_common.hpp"
#include "sparse_linear_algebra/matrix_multiplication/dense.hpp"
#include "sparse_linear_algebra/oblivious_map/oblivious_map.hpp"
#include "sparse_linear_algebra/util/time.h"
extern "C" {
#include <bcrandom.h>
}

// generates a partial permutation from `g` that maps `all_inner_indices`
// to random positions in [k], and also returns the array of unmapped positions
template <typename K, typename Generator>
std::pair<std::unordered_map<K, K>, std::vector<K>> permute_inner_indices(
    Generator &&g, const std::vector<K> &all_inner_indices, size_t k) {
  if (all_inner_indices.size() > k) {
    std::stringstream ss;
    ss << "k=" << k << ", but needs to be at least all_inner_indices.size()="
       << all_inner_indices.size() << "!";
    BOOST_THROW_EXCEPTION(std::invalid_argument(ss.str()));
  }
  std::vector<K> dense_indices(k);
  std::iota(dense_indices.begin(), dense_indices.end(), 0);
  std::shuffle(dense_indices.begin(), dense_indices.end(), g);
  std::unordered_map<K, K> perm;
  auto inner_it = all_inner_indices.begin();
  for (size_t i = 0; i < all_inner_indices.size(); i++, inner_it++) {
    perm.emplace(*inner_it, dense_indices[i]);
  }
  return std::make_pair(
      std::move(perm),
      std::vector<K>(dense_indices.begin() + all_inner_indices.size(),
                     dense_indices.end()));
}

template <
    typename Derived_A, typename Derived_B, typename K,
    typename T = typename Derived_A::Scalar,
    typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value,
                            int>::type = 0>
Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
matrix_multiplication_cols_rows(  // TODO: somehow derive row-/column sparsity
    const Eigen::SparseMatrixBase<Derived_A> &A_in,
    const Eigen::SparseMatrixBase<Derived_B> &B_in, oblivious_map<K, K> &prot,
    comm_channel &channel, int role,
    sparse_linear_algebra::matrix_multiplication::offline::TripleProvider<
        T, false> &triples,  // TODO: implement shared variant by pseudorandomly
                             // permuting indexes in another garbled circuit
    ssize_t chunk_size_in = -1, ssize_t k_A = -1,
    ssize_t k_B = -1,  // saves a communication round if set
    mpc_utils::Benchmarker *benchmarker = nullptr) {
  try {
    size_t k;
    std::vector<K> inner_indices;
    Eigen::SparseMatrix<T, Eigen::RowMajor> A;
    Eigen::SparseMatrix<T, Eigen::ColMajor> B;
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
        ret;
    // compute own indices and exchange k values if not given as arguments
    if (role == 0) {
      A = A_in.derived();
      auto inner_indices_copy = compute_inner_indices(A);
      inner_indices =
          std::vector<K>(inner_indices_copy.begin(), inner_indices_copy.end());
      if (k_A == -1) {
        k_A = inner_indices.size();
        channel.send(k_A);
      }
      if (k_B == -1) {
        channel.recv(k_B);
      }
    } else {
      B = B_in.derived();
      auto inner_indices_copy = compute_inner_indices(B);
      inner_indices =
          std::vector<K>(inner_indices_copy.begin(), inner_indices_copy.end());
      if (k_A == -1) {
        channel.recv(k_A);
      }
      if (k_B == -1) {
        k_B = inner_indices.size();
        channel.send(k_B);
      }
    }
    k = k_A + k_B;

    std::vector<std::pair<K, K>> perm(inner_indices.size());

    mpc_utils::Benchmarker::time_point start;
    if (benchmarker != nullptr) {
      start = benchmarker->StartTimer();
    }

    // generate correlated permutations; party with the higher number of indices
    // acts as the server of the ROOM protocol
    if ((k_A > k_B) == (role == 0)) {
      int seed;  // TODO: use wrapper around AES-based PRG from Obliv-C
      auto gen = newBCipherRandomGen();
      randomizeBuffer(gen, (char *)&seed, sizeof(int));
      releaseBCipherRandomGen(gen);
      std::mt19937 prg(seed);
      auto perm_result = permute_inner_indices(prg, inner_indices, k);
      // run ROOM protocol to give client their permutation
      prot.run_server(perm_result.first, perm_result.second, false,
                      benchmarker);
      boost::copy(perm_result.first, perm.begin());
    } else {
      std::vector<K> perm_values(role == 0 ? k_A : k_B);
      std::vector<K> inner_indices_copy(
          perm_values.size(),
          -1);  // Fill with dummy value by default that is guaranteed not to
                // match on the other side.
      std::copy_n(inner_indices.begin(),
                  std::min(inner_indices.size(), inner_indices_copy.size()),
                  inner_indices_copy.begin());
      prot.run_client(inner_indices_copy, perm_values, false, benchmarker);
      for (size_t i = 0; i < perm.size(); i++) {
        perm[i] = std::make_pair(inner_indices[i], perm_values[i]);
      }
    }

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("room_time", start);
      start = benchmarker->StartTimer();
    }

    // apply permutation and multiply
    if (role == 0) {
      Eigen::Matrix<T, Derived_A::RowsAtCompileTime,
                    Derived_A::ColsAtCompileTime>
          A_permuted(A.rows(), k);
      A_permuted.setZero();
      Eigen::SparseMatrix<T, Eigen::ColMajor> A_cols = A;
      for (auto pair : perm) {
        A_permuted.col(pair.second) = A_cols.col(pair.first);
      }

      if (benchmarker != nullptr) {
        benchmarker->AddSecondsSinceStart("permutation_time", start);
        start = benchmarker->StartTimer();
      }

      B.resize(k, B_in.cols());
      ret = matrix_multiplication_dense(A_permuted, B, channel, role, triples,
                                        chunk_size_in);

      if (benchmarker != nullptr) {
        benchmarker->AddSecondsSinceStart("dense_time", start);
      }
    } else {
      Eigen::Matrix<T, Derived_B::RowsAtCompileTime,
                    Derived_B::ColsAtCompileTime>
          B_permuted(k, B.cols());
      B_permuted.setZero();
      // copy into row major for faster row access
      Eigen::SparseMatrix<T, Eigen::RowMajor> B_rows = B;
      for (auto pair : perm) {
        B_permuted.row(pair.second) = B_rows.row(pair.first);
      }

      if (benchmarker != nullptr) {
        benchmarker->AddSecondsSinceStart("permutation_time", start);
        start = benchmarker->StartTimer();
      }

      A.resize(A_in.rows(), k);
      ret = matrix_multiplication_dense(A, B_permuted, channel, role, triples,
                                        chunk_size_in);

      if (benchmarker != nullptr) {
        benchmarker->AddSecondsSinceStart("dense_time", start);
        start = benchmarker->StartTimer();
      }
    }
    return ret;
  } catch (boost::exception &e) {
    e << error_a_size1(A_in.rows()) << error_a_size2(A_in.cols())
      << error_b_size1(B_in.rows()) << error_b_size2(B_in.cols());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
