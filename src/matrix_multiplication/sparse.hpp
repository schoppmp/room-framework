#include <unordered_map>
#include <unordered_set>
#include <boost/range/algorithm.hpp>
#include "dense.hpp"
#include "pir_protocol.hpp"
extern "C" {
  #include <bcrandom.h>
}

// computes all distinct inner indexes of m
template<typename T, int Opts, typename Index>
std::vector<size_t> compute_inner_indices(Eigen::SparseMatrix<T, Opts, Index>& m) {
  m.makeCompressed();
  std::unordered_set<size_t> indices(m.innerIndexPtr(), m.innerIndexPtr() + m.nonZeros());
  return std::vector<size_t>(indices.begin(), indices.end());
}

// maps the inner indexes of `m` to random indexes in [k],
template<typename T, int Opts, typename Index, typename Generator>
std::pair<std::unordered_map<size_t, size_t>, std::vector<size_t>>
permute_inner_indices(Generator&& g, Eigen::SparseMatrix<T, Opts, Index>& m,
  const std::vector<size_t>& all_inner_indices, size_t k
) {
  if(all_inner_indices.size() > k) {
    BOOST_THROW_EXCEPTION(std::invalid_argument("k needs to be at least all_inner_indices.size()"));
  }
  std::vector<size_t> dense_indices(k);
  std::iota(dense_indices.begin(), dense_indices.end(), 0);
  std::shuffle(dense_indices.begin(), dense_indices.end(), g);
  std::unordered_map<size_t, size_t> perm;
  auto inner_it = all_inner_indices.begin();
  for(size_t i = 0; i < all_inner_indices.size(); i++, inner_it++) {
    perm.emplace(*inner_it, dense_indices[i]);
  }
  return std::make_pair(std::move(perm), std::vector<size_t>(
    dense_indices.begin() + all_inner_indices.size(), dense_indices.end())
  );
}

template<typename Derived_A, typename Derived_B, typename K, typename V,
  typename T = typename Derived_A::Scalar, bool is_shared,
  typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value, int>::type = 0>
Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
matrix_multiplication( // TODO: somehow derive row-/column sparsity
    const Eigen::SparseMatrixBase<Derived_A>& A_in,
    const Eigen::SparseMatrixBase<Derived_B>& B_in,
    pir_protocol<K, V>& prot,
    comm_channel& channel, int role,
    triple_provider<T, is_shared>& triples,
    ssize_t chunk_size_in = -1,
    ssize_t k_A = -1, ssize_t k_B = -1 // saves a communication round if set
) {
  try {
    size_t k;
    if(role == 0) {
      Eigen::SparseMatrix<T, Eigen::RowMajor> A = A_in.derived();
      auto inner_indices = compute_inner_indices(A);
      if(k_A == -1) {
        k_A = inner_indices.size();
        channel.send(k_A);
      }
      if(k_B == -1) {
        channel.recv(k_B);
      }
      k = k_A + k_B;
      Eigen::SparseMatrix<T> B(k, B_in.cols());
      int seed; // TODO: use wrapper around AES-based PRG from Obliv-C
      auto gen = newBCipherRandomGen();
      randomizeBuffer(gen, (char*) &seed, sizeof(int));
      releaseBCipherRandomGen(gen);
      std::mt19937 prg(seed);
      auto perm_result = permute_inner_indices(prg, A, inner_indices, k);
      // run ROOM protocol to give client their permutation
      prot.run_server(perm_result.first, perm_result.second);
      // apply permutation locally
      Eigen::SparseMatrix<T> perm_A(A.cols(), k);
      std::vector<Eigen::Triplet<T>> perm_A_triplets;
      for(auto pair : perm_result.first) {
        perm_A_triplets.push_back(
          Eigen::Triplet<T>(pair.first, pair.second, 1));
      }
      perm_A.setFromTriplets(perm_A_triplets.begin(), perm_A_triplets.end());
      return matrix_multiplication(A * perm_A, B, channel, role, triples, chunk_size_in);
    } else {
      Eigen::SparseMatrix<T, Eigen::ColMajor> B = B_in.derived();
      auto inner_indices = compute_inner_indices(B);
      if(k_A == -1) {
        channel.recv(k_A);
      }
      if(k_B == -1) {
        k_B = inner_indices.size();
        channel.send(k_B);
      }
      k = k_A + k_B;
      Eigen::SparseMatrix<T> A(A_in.rows(), k);
      std::vector<size_t> perm_values(k_B);
      // get permutation from ROOM protocol
      prot.run_client(inner_indices, perm_values);
      // generate local permutation matrix
      Eigen::SparseMatrix<T> perm_B(k, B.rows());
      std::vector<Eigen::Triplet<T>> perm_B_triplets;
      for(size_t i = 0; i < k_B; i++) {
        perm_B_triplets.push_back(
          Eigen::Triplet<T>(perm_values[i], inner_indices[i], 1));
      }
      perm_B.setFromTriplets(perm_B_triplets.begin(), perm_B_triplets.end());
      return matrix_multiplication(A, perm_B * B, channel, role, triples, chunk_size_in);
    }
  } catch(boost::exception& e) {
    e << error_a_size1(A_in.rows()) << error_a_size2(A_in.cols())
      << error_b_size1(B_in.rows()) << error_b_size2(B_in.cols());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
