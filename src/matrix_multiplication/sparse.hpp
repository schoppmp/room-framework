#include <set>
#include <unordered_map>
#include "dense.hpp"

// computes all distinct inner indexes of m
template<typename T, int Opts, typename Index, typename Generator>
std::set<Index> compute_inner_indices(Eigen::SparseMatrix<T, Opts, Index>& m) {
  m.makeCompressed();
  return std::set<Index>(m.innerIndexPtr(), m.innerIndexPtr() + m.nonZeros());
}

// maps the inner indexes of `m` to random indexes in [k],
template<typename T, int Opts, typename Index, typename Generator>
std::unordered_map<Index, Index> permute_inner_indices(Generator&& g,
  Eigen::SparseMatrix<T, Opts, Index>& m,
  const std::set<Index>& all_inner_indices, size_t k;
) {
  if(all_inner_indices.size() > k) {
    BOOST_THROW_EXCEPTION(std::invalid_argument("k needs to be at least all_inner_indices.size()"))
  }
  std::vector<Index> dense_indices(k);
  std::iota(dense_indices.begin(), dense_indices.end(), Index(0));
  std::shuffle(dense_indices.begin(), dense_indices.end(), g);
  std::unordered_map<Index, Index> perm;
  auto inner_it = all_inner_indices.begin();
  for(size_t i = 0; i < all_inner_indices.size(); i++, inner_it++) {
    perm.emplace(*inner_it, dense_indices[i]);
  }
  for(auto it = m.innerIndexPtr(); it < m.innerIndexPtr() + m.nonZeros(); it++){
    *it = perm[*it];
  }
  return perm;
}

template<typename Derived_A, typename Derived_B,
  typename T = typename Derived_A::Scalar, bool is_shared,
  typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value, int>::type = 0>
Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
matrix_multiplication( // TODO: somehow derive row-/column sparsity
    const Eigen::SparseMatrixBase<Derived_A>& A_in,
    const Eigen::SparseMatrixBase<Derived_B>& B_in,
    comm_channel& channel, int role,
    triple_provider<T, is_shared>& triples,
    ssize_t chunk_size_in = -1,
    ssize_t k_A = -1, ssize_t k_B = -1 // saves a communication round if set
) {
  try {
    // copy into appropriate format; row-major (many zero columns) for A,
    // column-major for B
    Eigen::SparseMatrix<T, Eigen::RowMajor> A = A_in.derived();
    Eigen::SparseMatrix<T, Eigen::ColMajor> B = B_in.derived();

    // permute matrices
    size_t k;
    int seed = 12345; // TODO: use AES-based PRG from Obliv-C
    std::mt19937 prg(seed);
    if(role == 0) {
      auto inner_indices = compute_inner_indices(A);
      if(k_A == -1) {
        k_A = inner_indices.size();
        channel.send(k_A);
      }
      if(k_B) == -1) {
        channel.recv(k_B);
      }
      k = k_A + k_B;
      auto perm = permute_inner_indices(g, A, inner_indices, k);
      // TODO: use ROOM to generate permutation for client
    } else {
      auto inner_indices = compute_inner_indices(B);
      if(k_A == -1) {
        channel.recv(k_A);
      }
      if(k_B) == -1) {
        k_B = inner_indices.size();
        channel.send(k_B);
      }
      k = k_A + k_B;
      // TODO: get permutation from ROOM
    }

    } catch(boost::exception& e) {
      e << error_a_size1(A.rows()) << error_a_size2(A.cols())
        << error_b_size1(B.rows()) << error_b_size2(B.cols());
      e << error_chunk_size(chunk_size_in);
      throw;
    }
}
