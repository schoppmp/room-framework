#ifndef SPARSE_LINEAR_ALGEBRA_MATRIX_MULTIPLICATION_SPARSE_COMMON_HPP_
#define SPARSE_LINEAR_ALGEBRA_MATRIX_MULTIPLICATION_SPARSE_COMMON_HPP_

#include <unordered_set>
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "mpc_utils/canonical_errors.h"
#include "mpc_utils/statusor.h"

// TODO: remove this and only use ComputeInnerIndices instead.
// computes all distinct inner indexes of m
template <typename T, int Opts, typename Index>
std::vector<size_t> compute_inner_indices(
    Eigen::SparseMatrix<T, Opts, Index>& m) {
  m.makeCompressed();
  std::unordered_set<size_t> indices(m.innerIndexPtr(),
                                     m.innerIndexPtr() + m.nonZeros());
  return std::vector<size_t>(indices.begin(), indices.end());
}

// Compresses m, and returns the set of indexes into m's inner dimension where m
// has non-zero values.
template <typename T, int Opts, typename StorageIndex>
absl::flat_hash_set<StorageIndex> ComputeInnerIndices(
    Eigen::SparseMatrix<T, Opts, StorageIndex>* m) {
  m->makeCompressed();
  absl::flat_hash_set<StorageIndex> indices(m->innerIndexPtr(),
                                            m->innerIndexPtr() + m->nonZeros());
  return indices;
}

#endif  // SPARSE_LINEAR_ALGEBRA_MATRIX_MULTIPLICATION_SPARSE_COMMON_HPP_