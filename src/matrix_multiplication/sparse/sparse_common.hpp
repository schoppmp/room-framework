#pragma once

// computes all distinct inner indexes of m
template<typename T, int Opts, typename Index>
std::vector<size_t> compute_inner_indices(Eigen::SparseMatrix<T, Opts, Index>& m) {
  m.makeCompressed();
  std::unordered_set<size_t> indices(m.innerIndexPtr(), m.innerIndexPtr() + m.nonZeros());
  return std::vector<size_t>(indices.begin(), indices.end());
}
