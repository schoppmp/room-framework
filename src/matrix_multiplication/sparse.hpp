#include <unordered_map>
#include <set>
#include <boost/range/algorithm.hpp>
#include "dense.hpp"
#include "pir_protocol.hpp"

// computes all distinct inner indexes of m
template<typename T, int Opts, typename Index>
std::set<size_t> compute_inner_indices(Eigen::SparseMatrix<T, Opts, Index>& m) {
  m.makeCompressed();
  return std::set<size_t>(m.innerIndexPtr(), m.innerIndexPtr() + m.nonZeros());
}

// maps the inner indexes of `m` to random indexes in [k],
template<typename T, int Opts, typename Index, typename Generator>
std::pair<std::unordered_map<size_t, size_t>, std::vector<size_t>>
permute_inner_indices(Generator&& g, Eigen::SparseMatrix<T, Opts, Index>& m,
  const std::set<size_t>& all_inner_indices, size_t k
) {
  if(all_inner_indices.size() > k) {
    // std::cout << "all_inner_indices.size() = " << all_inner_indices.size() << "\n";
    BOOST_THROW_EXCEPTION(std::invalid_argument("k needs to be at least all_inner_indices.size()"));
  }
  std::vector<size_t> dense_indices(k);
  std::iota(dense_indices.begin(), dense_indices.end(), Index(0));
  std::shuffle(dense_indices.begin(), dense_indices.end(), g);
  std::unordered_map<size_t, size_t> perm;
  auto inner_it = all_inner_indices.begin();
  for(size_t i = 0; i < all_inner_indices.size(); i++, inner_it++) {
    perm.emplace(*inner_it, dense_indices[i]);
  }
  for(auto it = m.innerIndexPtr(); it < m.innerIndexPtr() + m.nonZeros(); it++){
    *it = perm[*it];
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
  // copy into appropriate format; row-major (many zero columns) for A,
  // column-major for B
  Eigen::SparseMatrix<T, Eigen::RowMajor> A = A_in.derived();
  Eigen::SparseMatrix<T, Eigen::ColMajor> B = B_in.derived();
  try {
    // permute matrices
    size_t k;
    if(role == 0) {
      auto inner_indices = compute_inner_indices(A);
      if(k_A == -1) {
        k_A = inner_indices.size();
        channel.send(k_A);
      }
      if(k_B == -1) {
        channel.recv(k_B);
      }
      k = k_A + k_B;
      int seed = 12345; // TODO: use AES-based PRG from Obliv-C
      std::mt19937 prg(seed);
      auto perm_result = permute_inner_indices(prg, A, inner_indices, k);
      prot.run_server(perm_result.first, perm_result.second);
    } else {
      auto inner_indices = compute_inner_indices(B);
      std::vector<size_t> inner_indices_vec(inner_indices.begin(), inner_indices.end());
      if(k_A == -1) {
        channel.recv(k_A);
      }
      if(k_B == -1) {
        k_B = inner_indices.size();
        channel.send(k_B);
      }
      k = k_A + k_B;
      std::vector<size_t> perm_values(k);
      // get permutation from ROOm protocol
      prot.run_client(inner_indices_vec, perm_values);
      // permute own matrix
      std::unordered_map<size_t, size_t> perm;
      for(size_t i = 0; i < k; i++) {
        perm.emplace(inner_indices_vec[i], perm_values[i]);
      }
      for(auto it = B.innerIndexPtr(); it < B.innerIndexPtr() + B.nonZeros(); it++){
        *it = perm[*it];
      }
    }
    A.resize(A.rows(), k);
    B.resize(k, B.cols());
    // now simply run a dense matrix multiplication
    return matrix_multiplication(A, B, channel, role, triples, chunk_size_in);
  } catch(boost::exception& e) {
    e << error_a_size1(A.rows()) << error_a_size2(A.cols())
      << error_b_size1(B.rows()) << error_b_size2(B.cols());
    e << error_chunk_size(chunk_size_in);
    throw;
  }
}
