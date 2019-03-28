#include "sparse_linear_algebra/util/randomize_matrix.hpp"
#include "mpc_utils/comm_channel.hpp"
extern "C" {
#include "obliv_common.h"
}

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {

template <typename T, bool is_shared>
OTTripleProvider<T, is_shared>::OTTripleProvider(int l, int m, int n,
                                                     int role,
                                                     comm_channel *chan,
                                                     int cap)
    : TripleProvider<T, is_shared>(l, m, n, role),
      triples_(cap),
      rng_(12345),  // TODO: Make this a random value
      chan_(chan) {}

template <typename T, bool is_shared>
Matrix<T> OTTripleProvider<T, is_shared>::GilboaProduct(
    Matrix<T> U, Matrix<T> V, bool input_assign) {

    // input_assign is 0 iff party 0 provides U and party 1 provides V
    size_t l = U.rows();
    size_t m = U.cols();
    size_t n = V.cols();
    if (m != V.rows()) {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Error: matrix dimensions don't agree"));
    }
    Matrix<T> R(l, n);
    R = Matrix<T>::Zero(l, n);
    size_t bw = sizeof(T) * 8;  // bitwidth of the shares
    // We run a single instance of N-COT_M, where N = n * l * m * bw
    // bw is the number of OTs needed for a single Gilboa multiplications,
    // and for multiplying U and V we needs l * n * m multiplications
    // with a naive algorithm.
    // M is the message size for such OTs, and M = 10 bytes in oblivc.
    // We will only use bw of the message space, which will be at most
    // 4 bytes in our case. This means that we are wasting 6 bytes, and could
    // exploit packing in a vectorized version of this code, to half the
    // comunication and running time. I did not implement this optimization.

    size_t N = n * l * m * bw;
    std::vector<T> opt0(sizeof(T) * N , 0);
    std::vector<T> opt1(sizeof(T) * N, 0);
    std::vector<T> cot_deltas(sizeof(T) * N, 0);
    std::vector<T> ot_result(sizeof(T) * N);
    bool *choices = reinterpret_cast<bool *>(calloc(N, sizeof(bool)));

    // Party holding U acts as sender
    int sender = (input_assign ? 0 : 1);
    int role = this->role();
    for (auto i = 0; i < l; ++i) {
        for (auto j = 0; j < n; j++) {
            for (auto z = 0; z < m; z++) {
                for (auto k = 0; k < bw; ++k) {
                    if (role == sender) {
                        cot_deltas[i*n*m*bw + j*m*bw + z*bw + k] = \
                          (((T)1) << k) * U(i, z);
                    } else {
                        choices[i*n*m*bw + j*m*bw + z*bw + k] = ((V(z, j) >> k) & 0x1);
                    }
    } } } }
    // Run OT Extension (Party holding U as sender)
    dhRandomInit();
    ProtocolDesc pd = {};
    if (chan_->connect_to_oblivc(pd) == -1) {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("run_server: connection failed"));
    }
    if (role == sender) {
      auto correlator = [](
        char *opt1_val, const char *opt0_val, int index, void *deltas) {
        T delta = ((reinterpret_cast<T *>(deltas)))[index];
        T *s1 = reinterpret_cast<T *>(opt1_val);
        T s0 = *((T *)(opt0_val));
        *s1 = delta + s0;
      };
      auto ot = honestOTExtSenderNew(&pd, 0);
      honestCorrelatedOTExtSend1Of2(ot,
        reinterpret_cast<char *>(opt0.data()),
        reinterpret_cast<char *>(opt1.data()),
        N,
        sizeof(T),
        correlator,
        cot_deltas.data());
      honestOTExtSenderRelease(ot);

    } else {
      auto ot = honestOTExtRecverNew(&pd, 0);
      honestCorrelatedOTExtRecv1Of2(ot,
          reinterpret_cast<char *>(ot_result.data()), choices,
          N, sizeof(T));
      honestOTExtRecverRelease(ot);
    }
    // Parties aggregates OT messages into their share of the result
    for (auto i = 0; i < l; i++) {
        for (auto j = 0; j < n; j++) {
            for (auto z = 0; z < m; z++) {
              for (auto k = 0; k < bw; k++) {
                if (role == sender) {
                  R(i, j) -= opt0[i*n*m*bw + j*m*bw + z*bw + k];
                } else {
                  R(i, j) += ot_result[i*n*m*bw + j*m*bw + z*bw + k];
                }
    } } } }
    cleanupProtocol(&pd);
    free(choices);
    return R;
}

template <typename T, bool is_shared>
void OTTripleProvider<T, is_shared>::Precompute(int num) {
  for (int i = 0; i < num; i++) {
    int l, m, n;
    std::tie(l, m, n) = this->dimensions();
    int role = this->role();
    // Party 1 generates a random l x m matrix, and
    // Party 2 generates a random m x n matrix.
    // We then run an OT-based matrix multiplication protocol
    // to produce a share (UV-R, R).
    // If is_shared is true then we repeat the above with
    // reversed roles to obtain (R', U'V'-R') and construct the final triple
    // as a share of (U+U')(V+V') = UV + UV' + U'V + U'V'
    // using some additional local computations.
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
