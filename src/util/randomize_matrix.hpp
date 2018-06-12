#pragma once
#include <Eigen/Dense>
#include <random>

// fills matrix with pseudorandomly generated elements
template<class Matrix, class Generator>
void randomize_matrix(Generator&& r, Matrix&& m) {
  std::uniform_int_distribution<typename std::remove_reference<Matrix>::type::Scalar> dist;
  for(size_t i = 0; i < m.rows(); i++) {
    for(size_t j = 0; j < m.cols(); j++) {
      m(i, j) = dist(r);
    }
  }
}
