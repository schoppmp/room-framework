#include "sparse_linear_algebra/util/randomize_matrix.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {

template <typename T, bool is_shared>
FakeTripleProvider<T, is_shared>::FakeTripleProvider(int l, int m, int n,
                                                     int role, int cap)
    : TripleProvider<T, is_shared>(l, m, n, role),
      triples_(cap),
      rng_(12345){};  // We seed the random number generator
                      // deterministically, which doesn't matter since we're
                      // generating insecure triples anyway.

template <typename T, bool is_shared>
void FakeTripleProvider<T, is_shared>::Precompute(int num) {
  for (int i = 0; i < num; i++) {
    int l, m, n;
    std::tie(l, m, n) = this->dimensions();
    int role = this->role();

    std::uniform_int_distribution<T> dist;
    Matrix<T> U(l, m), U_mask = Matrix<T>::Zero(l, m);
    Matrix<T> V(m, n), V_mask = Matrix<T>::Zero(m, n);
    Matrix<T> Z_mask(l, n);

    randomize_matrix(rng_, U);
    randomize_matrix(rng_, V);
    randomize_matrix(rng_, Z_mask);
    if (is_shared) {
      randomize_matrix(rng_, U_mask);
      randomize_matrix(rng_, V_mask);
    }

    if (role == 0) {
      triples_.push(std::make_tuple(U - U_mask, V_mask, Z_mask));
    } else {
      triples_.push(std::make_tuple(U_mask, V - V_mask, U * V - Z_mask));
    }
  }
}

template <typename T, bool is_shared>
Triple<T> FakeTripleProvider<T, is_shared>::GetTriple() {
  return triples_.pop();
}

}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra