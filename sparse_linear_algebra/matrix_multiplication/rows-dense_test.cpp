#include "sparse_linear_algebra/matrix_multiplication/rows-dense.hpp"
#include <thread>
#include "gtest/gtest.h"
#include "mpc_utils/testing/comm_channel_test_helper.hpp"
#include "sparse_linear_algebra/matrix_multiplication/offline/fake_triple_provider.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace {

template <typename T>
class RowsDenseTest : public ::testing::Test {
 protected:
  RowsDenseTest() : helper_(false) {}

  template <typename Derived_A, typename Derived_B>
  Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
  Multiply(const Eigen::SparseMatrixBase<Derived_A>& A,
           const Eigen::MatrixBase<Derived_B>& B, int k_A = -1,
           int chunk_size = -1) {
    int nonzero_rows = k_A;
    if (nonzero_rows == -1) {
      Eigen::SparseMatrix<T, Eigen::ColMajor> A_colmajor = A.derived();
      nonzero_rows = ComputeInnerIndices(&A_colmajor).size();
    }
    int l = A.rows(), m = A.cols(), n = B.cols();
    Eigen::Matrix<T, Derived_A::RowsAtCompileTime, Derived_B::ColsAtCompileTime>
        result_0, result_1;
    mpc_utils::comm_channel* channel_0 = helper_.GetChannel(0);
    mpc_utils::comm_channel* channel_1 = helper_.GetChannel(1);
    std::thread thread1(
        [&result_1, &B, nonzero_rows, l, m, n, k_A, chunk_size, channel_1] {
          offline::FakeTripleProvider<T, false> triples(nonzero_rows, m, n, 1);
          triples.Precompute(1);
          Eigen::SparseMatrix<T> A_zero(l, m);
          result_1 = matrix_multiplication_rows_dense(A_zero, B, *channel_1, 1,
                                                      triples, chunk_size, k_A);
          channel_1->flush();
        });
    offline::FakeTripleProvider<T, false> triples(nonzero_rows, m, n, 0);
    triples.Precompute(1);
    result_0 = matrix_multiplication_rows_dense(
        A, Derived_B::Zero(m, n), *channel_0, 0, triples, chunk_size, k_A);
    thread1.join();
    return result_0 + result_1;
  }

  mpc_utils::testing::CommChannelTestHelper helper_;
};

using MyTypes = ::testing::Types<uint8_t, uint16_t, uint32_t, uint64_t>;
TYPED_TEST_SUITE(RowsDenseTest, MyTypes);

TYPED_TEST(RowsDenseTest, TestSimple) {
  const int l = 4, m = 3, n = 1;
  Eigen::SparseMatrix<TypeParam> A(l, m);
  Eigen::Matrix<TypeParam, m, n> B;
  Eigen::Matrix<TypeParam, l, n> result;
  A.insert(0, 1) = 4;
  A.insert(1, 2) = 5;
  B << 0, 1, 1;
  result << 4, 5, 0, 0;

  // Automatic sparsity.
  EXPECT_EQ(this->Multiply(A, B), result);

  // Explicit sparsity.
  EXPECT_EQ(this->Multiply(A, B, 2), result);

  // Explicit sparsity larger than actual sparsity.
  EXPECT_EQ(this->Multiply(A, B, 3), result);
}

TYPED_TEST(RowsDenseTest, TestZero) {
  const int l = 1, m = 1, n = 1;
  Eigen::SparseMatrix<TypeParam> A(l, m);
  Eigen::Matrix<TypeParam, m, n> B;
  Eigen::Matrix<TypeParam, l, n> result;
  B << 0;
  result << 0;
  EXPECT_EQ(this->Multiply(A, B), result);
}

}  // namespace
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra