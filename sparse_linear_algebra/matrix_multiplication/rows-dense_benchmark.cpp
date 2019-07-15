#include <thread>
#include "benchmark/benchmark.h"
#include "mpc_utils/testing/comm_channel_test_helper.hpp"
#include "sparse_linear_algebra/matrix_multiplication/offline/fake_triple_provider.hpp"
#include "sparse_linear_algebra/matrix_multiplication/rows-dense.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace {

class RowsDenseBenchmarkHelper {
 public:
  RowsDenseBenchmarkHelper() : comm_channel_helper_(false) {}

  template <typename Derived_A, typename Derived_B,
            typename T = typename Derived_A::Scalar>
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
    mpc_utils::comm_channel* channel_0 = comm_channel_helper_.GetChannel(0);
    mpc_utils::comm_channel* channel_1 = comm_channel_helper_.GetChannel(1);
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

 private:
  mpc_utils::testing::CommChannelTestHelper comm_channel_helper_;
};

static void BM_Small(benchmark::State& state) {
  const int l = state.range(0), m = 12, n = 1;
  Eigen::SparseMatrix<uint64_t> A(l, m);
  for (int i = 0; i < state.range(1); i++) {
    // Distribute nonzeros across matrix.
    A.insert((i * 91) % l, (i * 101) % m) = i;
  }
  Eigen::Matrix<uint64_t, m, n> B;
  B << 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0;
  RowsDenseBenchmarkHelper helper;

  for (auto _ : state) {
    auto result = helper.Multiply(A, B, state.range(1));
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Small)->Ranges({{1 << 10, 1 << 20}, {1, 100}});

}  // namespace
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra