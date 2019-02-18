// A FakeTripleProvider generates multiplication triples using a shared random
// seed. This is an insecure implementation of TripleProvider that should not be
// used for production code, but can be useful when  measuring only the online
// running time of protocols.

#pragma once

#include <random>
#include "sparse_linear_algebra/matrix_multiplication/offline/triple_provider.hpp"
#include "sparse_linear_algebra/util/blocking_queue.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {

template <typename T, bool is_shared = false>
class FakeTripleProvider : public virtual TripleProvider<T, is_shared> {
 public:
  // Constructs a triple provider for multiplying an (l x m) with an (m x n)
  // matrix. The `cap` argument allows to use a bounded queue for storing
  // triples.
  FakeTripleProvider(int l, int m, int n, int role, int cap = -1);

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
};

}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra

#include "fake_triple_provider.tpp"