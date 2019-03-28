/*
 An implementation of secure 2-party triple generation based on oblivious transfer.
 It is similar to Gilboa's method for oblivious multiplication, but with
 some optimizations specific to matrix multiplication, namely vectorization.
*/

#pragma once

#include <random>
#include "mpc_utils/comm_channel.hpp"
#include "sparse_linear_algebra/matrix_multiplication/offline/triple_provider.hpp"
#include "sparse_linear_algebra/util/blocking_queue.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {

template <typename T, bool is_shared = false>
class OTTripleProvider : public virtual TripleProvider<T, is_shared> {
 public:
  // Constructs a triple provider for multiplying an (l x m) with an (m x n)
  // matrix. The `cap` argument allows to use a bounded queue for storing
  // triples.
  OTTripleProvider(int l, int m, int n, int role, comm_channel *chan,
      int cap = -1);

  // Precomputes a number of triples. Blocks if capacity is bounded and queue is
  // full.
  void Precompute(int num);

  // Returns a precomputed triple. Can be called concurrently with precompute();
  // however, only one thread may call get() at the same time.
  Triple<T> GetTriple() override;

 private:
  // Precomputed triples to be returned by GetTriple().
  blocking_queue<Triple<T>> triples_;

  // Random number generator used for creating triples.
  std::mt19937 rng_;

  // Communication channel for the triple generation.
  comm_channel *chan_;

  // Returns an additive share of UV, where
  // input_assign is 0 iff party 0 provides U and party 1 provides V
  Matrix<T> GilboaProduct(Matrix<T> U, Matrix<T> V, bool input_assign);
};

}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra

#include "ot_triple_provider.tpp"
