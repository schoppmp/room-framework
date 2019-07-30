#include <iostream>
#include <vector>
#include "absl/memory/memory.h"
#include "boost/container/vector.hpp"
#include "boost/throw_exception.hpp"
#include "emp-ot/emp-ot.h"
#include "emp-tool/emp-tool.h"
#include "mpc_utils/canonical_errors.h"
#include "mpc_utils/comm_channel_emp_adapter.hpp"
#include "mpc_utils/status_macros.h"
#include "sparse_linear_algebra/util/randomize_matrix.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {

template <typename T, bool is_shared>
mpc_utils::StatusOr<std::unique_ptr<OTTripleProvider<T, is_shared>>>
OTTripleProvider<T, is_shared>::Create(int l, int m, int n, int role,
                                       comm_channel *channel, int cap) {
  if (l < 1 || m < 1 || n < 1) {
    return mpc_utils::InvalidArgumentError("l, m and n must be positive");
  }
  if (role != 0 && role != 1) {
    return mpc_utils::InvalidArgumentError("role must be 0 or 1");
  }
  // Create EMP adapter. Use a direct connection if channel is not measured.
  channel->sync();
  ASSIGN_OR_RETURN(auto adapter, mpc_utils::CommChannelEMPAdapter::Create(
                                     channel, !channel->is_measured()));
  return absl::WrapUnique(
      new OTTripleProvider(l, m, n, role, std::move(adapter), cap));
}

template <typename T, bool is_shared>
OTTripleProvider<T, is_shared>::OTTripleProvider(
    int l, int m, int n, int role,
    std::unique_ptr<mpc_utils::CommChannelEMPAdapter> channel_adapter, int cap)
    : TripleProvider<T, is_shared>(l, m, n, role),
      triples_(cap),
      channel_adapter_(std::move(channel_adapter)) {}

template <typename T, bool is_shared>
Matrix<T> OTTripleProvider<T, is_shared>::GilboaProduct(Matrix<T> U,
                                                        Matrix<T> V,
                                                        bool input_assign) {
  // input_assign is 0 iff party 0 provides U and party 1 provides V
  int64_t l = U.rows();
  int64_t m = U.cols();
  int64_t n = V.cols();
  if (m != V.rows()) {
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Error: matrix dimensions don't agree"));
  }

  Matrix<uint64_t> R = Matrix<uint64_t>::Zero(l, n);
  int64_t bit_width = sizeof(uint64_t) * 8;  // bitwidth of the shares.
  // We run a single instance of N-COT_M, where N = n * ceil(l/2) * m *
  // bit_width. bit_width is the number of OTs needed for a single Gilboa
  // multiplications, and for multiplying U and V we needs l * n * m
  // multiplications with a naive algorithm. The ceil(l/2) above comes from the
  // fact that we exploit packing, as M in EMP is 128 bits. This optimization
  // halves computation and communication, and assumes that sizeof(T) = 64.

  // We use cot_add_delta fro EMP. This is a particular case of N-COT where the
  // sender defines cot_deltas as a list of N pairs of uint64_t. The receiver
  // gets a block (128 bits) that is the component-wise sum of cot_deltas[i] and
  // m_0 (seen as a pair of uint64_t) if its choose bit is 1, and m_0 otherwise.
  int64_t N = n * ((l + 1) / 2) * m * bit_width;

  std::vector<emp::block> opt0(N);
  std::vector<emp::block> ot_result(N);
  // std::vector<bool> is implemented as a bitstring, ans thus does not use a
  // bool * internally, which EMP requires.
  boost::container::vector<bool> choices(N);
  std::vector<std::pair<uint64_t, uint64_t>> cot_deltas(N);

  // Party holding U acts as sender
  int sender = (input_assign ? 0 : 1);
  int role = this->role();
  auto stride = (l + 1) / 2;
  // Construct list of deltas and choices for the N COTs to compute an l x n x
  // m matrix multiplication between matrices U and V. These are in principle
  // l*n*m multiplications between every pair of values a, b in U and V, each
  // requiring bit_width COTs with correlation function f(x) = 2^i * a + x and
  // choice but b[i] (ith bit of the binary encoding of b). However, we can pack
  // several multiplications with the same value, such as (a1 * b), (a2 * b), in
  // a single OT, to reduce from l*n*m to stride*n*m Gilboa multiplications The
  // correlation function operates on a1 and a2 simultaneouly, resulting in f(x)
  // = 2^i * a1 + x[0:63] || 2^i * a1 + x[64:128], as x is an emp::block (AES
  // block) of length 128).
  for (int64_t i = 0; i < stride; ++i) {
    for (int64_t j = 0; j < n; j++) {
      for (int64_t z = 0; z < m; z++) {
        for (int k = 0; k < bit_width; k++) {
          if (role == sender) {
            cot_deltas[i * n * m * bit_width + j * m * bit_width +
                       z * bit_width + k] =
                std::make_pair(
                    (((uint64_t)1) << k) * U(i, z),
                    (i + stride < l) ? (((T)1) << k) * U(i + stride, z) : 0);
          } else {
            choices[i * n * m * bit_width + j * m * bit_width + z * bit_width +
                    k] = ((V(z, j) >> k) & 0x1);
          }
        }
      }
    }
  }

  // Run OT Extension (Party holding U as sender)
  emp::SHOTExtension<mpc_utils::CommChannelEMPAdapter> ot(
      channel_adapter_.get());
  if (role == sender) {
    ot.send_cot_add_delta(opt0.data(), cot_deltas.data(), N);
  } else {
    ot.recv_cot(ot_result.data(), choices.data(), N);
  }
  channel_adapter_->flush();
  // Parties aggregate OT messages into their share of the result
  for (int64_t i = 0; i < stride; i++) {
    for (int64_t j = 0; j < n; j++) {
      for (int64_t z = 0; z < m; z++) {
        for (int k = 0; k < bit_width; k++) {
          if (role == sender) {
            R(i, j) -=
                (uint64_t)(opt0[i * n * m * bit_width + j * m * bit_width +
                                z * bit_width + k][0]);
            if (i + stride < l) {
              R(i + stride, j) -=
                  (uint64_t)(opt0[i * n * m * bit_width + j * m * bit_width +
                                  z * bit_width + k][1]);
            }
          } else {
            R(i, j) +=
                (uint64_t)(ot_result[i * n * m * bit_width + j * m * bit_width +
                                     z * bit_width + k][0]);
            if (i + stride < l) {
              R(i + stride, j) += (uint64_t)(
                  ot_result[i * n * m * bit_width + j * m * bit_width +
                            z * bit_width + k][1]);
            }
          }
        }
      }
    }
  }
  return R;
}

template <typename T, bool is_shared>
void OTTripleProvider<T, is_shared>::Precompute(int num) {
  for (auto i = 0; i < num; i++) {
    int l, m, n;
    std::tie(l, m, n) = this->dimensions();
    int role = this->role();
    // Role 0 generates a random l x m matrix U0, and Role 1 generates a random
    // m x n matrix V1. We then run an OT-based matrix multiplication protocol
    // to produce a share of U0 * V1. If is_shared is true then we repeat the
    // above with reversed roles and inputs V0, U1 and construct the final
    // triple as a share of (U0+U1)(V0+V1) = U0V0 + U0V1 + U1V0 + U1V1 using
    // some additional local computations.
    Matrix<T> U = Matrix<T>::Zero(l, m);
    Matrix<T> V = Matrix<T>::Zero(m, n);
    if (role == 0) {
      randomize_matrix(rng_, U);
    } else {
      randomize_matrix(rng_, V);
    }
    Matrix<T> R = GilboaProduct(U, V, true);  // Compute share of U0 * V1 above.

    // Repeat the above if we want a shared triple.
    if (is_shared) {
      if (role == 0) {
        randomize_matrix(rng_, V);
      } else {
        randomize_matrix(rng_, U);
      }
      R += U * V + GilboaProduct(U, V, false);
    }
    triples_.push(std::make_tuple(U, V, R));
  }
}

template <typename T, bool is_shared>
Triple<T> OTTripleProvider<T, is_shared>::GetTriple() {
  return triples_.pop();
}

}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra
