#include <iostream>
#include <vector>
#include "boost/container/vector.hpp"
#include "boost/throw_exception.hpp"
#include "emp-ot/emp-ot.h"
#include "emp-tool/emp-tool.h"
#include "sparse_linear_algebra/util/randomize_matrix.hpp"
extern "C" {
#include "obliv_common.h"
}

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {

template <typename T, bool is_shared>
OTTripleProvider<T, is_shared>::OTTripleProvider(int l, int m, int n, int role,
                                                 emp::NetIO *io, int cap)
    : TripleProvider<T, is_shared>(l, m, n, role),
      triples_(cap),
      rng_(12345),  // TODO: Make this a random value
      chan_(io) {}

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
  int64_t bw = sizeof(uint64_t) * 8;  // bitwidth of the shares.
  // We run a single instance of N-COT_M, where N = n * ceil(l/2) * m * bw. bw
  // is the number of OTs needed for a single Gilboa multiplications, and for
  // multiplying U and V we needs l * n * m multiplications with a naive
  // algorithm. The ceil(l/2) above comes from the fact that we exploit packing,
  // as M in EMP is 128 bits. This optimization halves computation and
  // communication, and assumes that sizeof(T) = 64.

  // We use cot_add_delta fro EMP. This is a particular case of N-COT where the
  // sender defines cot_deltas as a list of N pairs of uint64_t. The receiver
  // gets a block (128 bits) that is the component-wise sum of cot_deltas[i] and
  // m_0 (seen as a pair of uint64_t) if its choose bit is 1, and m_0 otherwise.
  auto N = n * ((l + 1) / 2) * m * bw;

  std::vector<emp::block> opt0(N);
  std::vector<emp::block> ot_result(N);
  // std::vector<bool> is implemented as a bitstring, ans thus
  // does not use a bool * internally, which EMP requires
  boost::container::vector<bool> choices(N);
  std::vector<std::pair<uint64_t, uint64_t> > cot_deltas(N);

  // Party holding U acts as sender
  int sender = (input_assign ? 0 : 1);
  int role = this->role();
  auto stride = (l + 1) / 2;
  // Construct list of deltas and choices for the N COTs to compute and l x n x
  // m matrix multiplication between matrices U and V These are in principle
  // l*n*m multiplications between every pair of values a, b in U and V, each
  // requiring bw COTs with correlation function f(x) = 2^i * a + x and choice
  // but b[i] (ith bit of the binary encoding of b) However we can pack several
  // multiplications with the same value, such as (a1 * b), (a2 * b), in a
  // single OT, to reduce from l*n*m to stride*n*m Gilboa multiplications The
  // correlation function operates on a1 and a2 simultaneouly, resulting in f(x)
  // = 2^i * a1 + x[0:63] || 2^i * a1 + x[64:128], as x is an emp::block (AES
  // block) of length 128).
  for (auto i = 0; i < stride; ++i) {
    for (auto j = 0; j < n; j++) {
      for (auto z = 0; z < m; z++) {
        for (auto k = 0; k < bw; k++) {
          if (role == sender) {
            cot_deltas[i * n * m * bw + j * m * bw + z * bw + k] =
                std::make_pair(
                    (((uint64_t)1) << k) * U(i, z),
                    (i + stride < l) ? (((T)1) << k) * U(i + stride, z) : 0);
          } else {
            choices[i * n * m * bw + j * m * bw + z * bw + k] =
                ((V(z, j) >> k) & 0x1);
          }
        }
      }
    }
  }

  // Run OT Extension (Party holding U as sender)
  chan_->sync();
  emp::SHOTExtension<emp::NetIO> ot(chan_);
  if (role == sender) {
    ot.send_cot_add_delta(opt0.data(), cot_deltas.data(), N);
  } else {
    ot.recv_cot(ot_result.data(), choices.data(), N);
  }
  chan_->flush();
  // Parties aggregate OT messages into their share of the result
  for (auto i = 0; i < stride; i++) {
    for (auto j = 0; j < n; j++) {
      for (auto z = 0; z < m; z++) {
        for (auto k = 0; k < bw; k++) {
          if (role == sender) {
            R(i, j) -=
                (uint64_t)(opt0[i * n * m * bw + j * m * bw + z * bw + k][0]);
            if (i + stride < l)
              R(i + stride, j) -=
                  (uint64_t)(opt0[i * n * m * bw + j * m * bw + z * bw + k][1]);
          } else {
            R(i, j) += (uint64_t)(
                ot_result[i * n * m * bw + j * m * bw + z * bw + k][0]);
            if (i + stride < l)
              R(i + stride, j) += (uint64_t)(
                  ot_result[i * n * m * bw + j * m * bw + z * bw + k][1]);
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
    // Party 1 generates a random l x m matrix, and Party 2 generates a random m
    // x n matrix. We then run an OT-based matrix multiplication protocol to
    // produce a share (UV-R, R). If is_shared is true then we repeat the above
    // with reversed roles to obtain (R', U'V'-R') and construct the final
    // triple as a share of (U+U')(V+V') = UV + UV' + U'V + U'V' using some
    // additional local computations.
    Matrix<T> U(l, m);
    Matrix<T> V(m, n);
    randomize_matrix(rng_, U);
    randomize_matrix(rng_, V);
    Matrix<T> R = GilboaProduct(U, V, 0);  // Compute share of UV' above
    if (!is_shared) {
      if (role == 0) {
        V = Matrix<T>::Zero(m, n);
      } else {
        U = Matrix<T>::Zero(l, m);
      }
    } else {
      Matrix<T> R2 = GilboaProduct(U, V, 1);  // Compute share of U'V above
      // Compute final shares of UV from local cross products
      R = R + R2 + U * V;
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
