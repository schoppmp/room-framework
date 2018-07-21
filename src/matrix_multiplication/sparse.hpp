#include <unordered_map>
#include <unordered_set>
#include <boost/range/algorithm.hpp>
#include <Eigen/Sparse>
#include "dense.hpp"
#include "pir_protocol.hpp"
#include "util/time.h"
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

// generates a partial permutation from `g` that maps `all_inner_indices`
// to random positions in [k], and also returns the array of unmapped positions
template<typename Generator>
std::pair<std::unordered_map<size_t, size_t>, std::vector<size_t>>
permute_inner_indices(Generator&& g,
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
  typename T = typename Derived_A::Scalar,
  typename std::enable_if<std::is_same<T, typename Derived_B::Scalar>::value, int>::type = 0>
Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
matrix_multiplication( // TODO: somehow derive row-/column sparsity
    const Eigen::SparseMatrixBase<Derived_A>& A_in,
    const Eigen::SparseMatrixBase<Derived_B>& B_in,
    pir_protocol<K, V>& prot,
    comm_channel& channel, int role,
    triple_provider<T, false>& triples, // TODO: implement shared variant by pseudorandomly permuting indexes in another garbled circuit
    ssize_t chunk_size_in = -1,
    ssize_t k_A = -1, ssize_t k_B = -1, // saves a communication round if set
    bool print_times = false
) {
  try {
    size_t k;
    std::vector<size_t> inner_indices;
    Eigen::SparseMatrix<T, Eigen::RowMajor> A;
    Eigen::SparseMatrix<T, Eigen::ColMajor> B;
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime> ret;
    // compute own indices and exchange k values if not given as arguments
    if(role == 0) {
      A = A_in.derived();
      inner_indices = compute_inner_indices(A);
      if(k_A == -1) {
        k_A = inner_indices.size();
        channel.send(k_A);
      }
      if(k_B == -1) {
        channel.recv(k_B);
      }
    } else {
      B = B_in.derived();
      inner_indices = compute_inner_indices(B);
      if(k_A == -1) {
        channel.recv(k_A);
      }
      if(k_B == -1) {
        k_B = inner_indices.size();
        channel.send(k_B);
      }
    }
    k = k_A + k_B;
    std::vector<std::pair<size_t, size_t>> perm(inner_indices.size());
    // generate correlated permutations; party with the higher number of indices
    // acts as the server of the ROOM protocol
    if((k_A > k_B) == (role == 0)) {
      double start = 0;
      if(print_times) {
        start = timestamp();
      }
      int seed; // TODO: use wrapper around AES-based PRG from Obliv-C
      auto gen = newBCipherRandomGen();
      randomizeBuffer(gen, (char*) &seed, sizeof(int));
      releaseBCipherRandomGen(gen);
      std::mt19937 prg(seed);
      auto perm_result = permute_inner_indices(prg, inner_indices, k);
      // run ROOM protocol to give client their permutation
      prot.run_server(perm_result.first, perm_result.second);
      boost::copy(perm_result.first, perm.begin());
      if(print_times) {
        double end = timestamp();
        std::cout << "reduction_time: " << end - start << " s\n";
      }
    } else {
      double start = 0;
      if(print_times) {
        start = timestamp();
      }
      std::vector<size_t> perm_values(role == 0 ? k_A : k_B);
      prot.run_client(inner_indices, perm_values);
      boost::copy(combine_pair(inner_indices, perm_values), perm.begin());
      if(print_times) {
        double end = timestamp();
        std::cout << "reduction_time: " << end - start << " s\n";
      }
    }
    // apply permutation and multiply
    if(role == 0) {
      Eigen::SparseMatrix<T, Eigen::ColMajor> perm_A(A.cols(), k);
      // generate local permutation matrix
      std::vector<size_t> sizes(k, 1);
      perm_A.reserve(sizes);
      for(auto pair : perm) {
        perm_A.insert(pair.first, pair.second) = 1;
      }
      B.resize(k, B_in.cols());
      double start = 0;
      if(print_times) {
        start = timestamp();
      }
      ret = matrix_multiplication(A * perm_A, B, channel, role, triples, chunk_size_in);
      if(print_times) {
        double end = timestamp();
        std::cout << "dense_time: " << end - start << " s\n";
      }
    } else {
      Eigen::SparseMatrix<T, Eigen::RowMajor> perm_B(k, B.rows());
      // generate local permutation matrix
      std::vector<size_t> sizes(k, 1);
      perm_B.reserve(sizes);
      for(auto pair: perm) {
        perm_B.insert(pair.second, pair.first) = 1;
      }
      A.resize(A_in.rows(), k);
      double start = 0;
      if(print_times) {
        start = timestamp();
      }
      ret = matrix_multiplication(A, perm_B * B, channel, role, triples, chunk_size_in);
      if(print_times) {
        double end = timestamp();
        std::cout << "dense_time: " << end - start << " s\n";
      }
    }
    return ret;
  } catch(boost::exception& e) {
    e << error_a_size1(A_in.rows()) << error_a_size2(A_in.cols())
      << error_b_size1(B_in.rows()) << error_b_size2(B_in.cols());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
