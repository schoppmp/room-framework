// An implementation of secure 2-party triple generation based on oblivious
// transfer. It is similar to Gilboa's method for oblivious multiplication, but
// with some optimizations specific to matrix multiplication, namely
// vectorization.

#pragma once

#include <random>
#include "emp-ot/emp-ot.h"
#include "emp-tool/emp-tool.h"
#include "mpc_utils/comm_channel.hpp"
#include "mpc_utils/comm_channel_emp_adapter.hpp"
#include "mpc_utils/openssl_uniform_bit_generator.hpp"
#include "mpc_utils/statusor.h"
#include "sparse_linear_algebra/matrix_multiplication/offline/triple_provider.hpp"
#include "sparse_linear_algebra/util/blocking_queue.hpp"
#include "mpc_utils/openssl_uniform_bit_generator.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {

template <typename T, bool is_shared = false>
class OTTripleProvider : public virtual TripleProvider<T, is_shared> {
  static_assert(sizeof(T) == 8,
                "OtTripleProvider only supports 64-bit integers at the moment");

 public:
  // Constructs a triple provider for multiplying an (l x m) with an (m x n)
  // matrix. The `cap` argument allows to use a bounded queue for storing
  // triples.
  static mpc_utils::StatusOr<std::unique_ptr<OTTripleProvider<T, is_shared>>>
  Create(int l, int m, int n, int role, comm_channel *channel, int cap = -1);

  // Precomputes a number of triples. Blocks if capacity is bounded and queue is
  // full.
  void Precompute(int num);

  // Returns a precomputed triple. Can be called concurrently with precompute();
  // however, only one thread may call get() at the same time.
  Triple<T> GetTriple() override;

 private:
  // Private constructor, called by Create.
  OTTripleProvider(
      int l, int m, int n, int role,
      std::unique_ptr<mpc_utils::CommChannelEMPAdapter> channel_adapter,
      int cap);

  // Returns an additive share of UV, where
  // input_assign is 0 iff party 0 provides U and party 1 provides V
  Matrix<T> GilboaProduct(Matrix<T> U, Matrix<T> V, bool input_assign);

  // Precomputed triples to be returned by GetTriple().
  blocking_queue<Triple<T>> triples_;

  // Wrapper around the communication channel passed at construction.
  // Used for oblivious transfers during triple generation.
  std::unique_ptr<mpc_utils::CommChannelEMPAdapter> channel_adapter_;

  // Random number generator for creating triples.
  mpc_utils::OpenSSLUniformBitGenerator rng_;
};

}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra

#include "ot_triple_provider.tpp"
