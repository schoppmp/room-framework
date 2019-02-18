// A TripleProvider is used to compute multiplication triples for matrix
// multiplications. A matrix multiplication triple consist of three additively
// shared matrices U, V, Z, where UV = Z. For details on different methods for
// generating matrix triples, see the SecureML paper:
// https://eprint.iacr.org/2017/396

#pragma once

#include "Eigen/Dense"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {

template <typename T>
using Matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;

template <typename T>
using Triple = std::tuple<Matrix<T>, Matrix<T>, Matrix<T>>;

template <typename T, bool is_shared = false>
class TripleProvider {
 public:
  // Creates a new TripleProvider for multiplying (l x m)- with (m x
  // n)-matrices. Role can be 0 or 1, and must match the role specified in the
  // matrix multiplication call.
  TripleProvider(int l, int m, int n, int role)
      : dimensions_(l, m, n), role_(role){};

  // Returns (l, m, n) passed at construction as a tuple.
  std::tuple<int, int, int> dimensions() { return dimensions_; }

  // Returns this TripleProvider's role.
  int role() { return role_; }

  // Returns a single triple.
  virtual Triple<T> GetTriple() = 0;

 private:
  // The matrix dimensions passed at construction.
  std::tuple<int, int, int> dimensions_;

  // The role of the party that owns this TripleProvider.
  int role_;
};

}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra
