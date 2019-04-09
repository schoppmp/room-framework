#include "sparse_linear_algebra/matrix_multiplication/offline/ot_triple_provider.hpp"
#include <thread>
#include "absl/memory/memory.h"
#include "emp-ot/emp-ot.h"
#include "emp-tool/emp-tool.h"
#include "gtest/gtest.h"
#include "mpc_utils/boost_serialization/eigen.hpp"
#include "mpc_utils/comm_channel.hpp"
#include "mpc_utils/status_matchers.h"
#include "mpc_utils/testing/comm_channel_test_helper.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {
namespace {

class OTTripleProviderTest : public ::testing::Test {
 protected:
  OTTripleProviderTest() : helper_(false) {}
  void SetUp(int l, int m, int n) {
    using DistributedTripleProvider = OTTripleProvider<uint64_t, false>;
    using SharedTripleProvider = OTTripleProvider<uint64_t, true>;
    comm_channel *chan0 = helper_.GetChannel(0);
    comm_channel *chan1 = helper_.GetChannel(1);
    std::thread thread1([this, chan1, l, m, n] {
      ASSERT_OK_AND_ASSIGN(
          distributed_triples_1_,
          DistributedTripleProvider::Create(l, m, n, 1, chan1));
      ASSERT_OK_AND_ASSIGN(shared_triples_1_,
                           SharedTripleProvider::Create(l, m, n, 1, chan1));
    });
    ASSERT_OK_AND_ASSIGN(distributed_triples_0_,
                         DistributedTripleProvider::Create(l, m, n, 0, chan0));
    ASSERT_OK_AND_ASSIGN(shared_triples_0_,
                         SharedTripleProvider::Create(l, m, n, 0, chan0));
    thread1.join();
  }

  mpc_utils::testing::CommChannelTestHelper helper_;
  std::unique_ptr<OTTripleProvider<uint64_t, false>> distributed_triples_0_;
  std::unique_ptr<OTTripleProvider<uint64_t, true>> shared_triples_0_;
  std::unique_ptr<OTTripleProvider<uint64_t, false>> distributed_triples_1_;
  std::unique_ptr<OTTripleProvider<uint64_t, true>> shared_triples_1_;
};

TEST_F(OTTripleProviderTest, DistributedTriples) {
  Matrix<uint64_t> u, v, w0, w1;
  for (int l = 1; l < 5; l++) {
    for (int m = 1; m < 5; m++) {
      for (int n = 1; n < 5; n++) {
        SetUp(l, m, n);
        std::thread thread1([this, &v, &w1] {
          distributed_triples_1_->Precompute(1);
          std::tie(std::ignore, v, w1) = distributed_triples_1_->GetTriple();
        });
        distributed_triples_0_->Precompute(1);
        std::tie(u, std::ignore, w0) = distributed_triples_0_->GetTriple();
        thread1.join();

        EXPECT_EQ(u.rows(), l);
        EXPECT_EQ(u.cols(), m);
        EXPECT_EQ(v.rows(), m);
        EXPECT_EQ(v.cols(), n);
        EXPECT_EQ(w0.rows(), l);
        EXPECT_EQ(w1.rows(), l);

        EXPECT_EQ(w0.cols(), n);
        EXPECT_EQ(w1.cols(), n);
        EXPECT_EQ(u * v, w0 + w1);
      }
    }
  }
}

TEST_F(OTTripleProviderTest, SharedTriples) {
  Matrix<uint64_t> u0, u1, v0, v1, w0, w1;
  for (int l = 1; l < 5; l++) {
    for (int m = 1; m < 5; m++) {
      for (int n = 1; n < 5; n++) {
        SetUp(l, m, n);
        std::thread thread1([this, &u1, &v1, &w1] {
          shared_triples_1_->Precompute(1);
          std::tie(u1, v1, w1) = shared_triples_1_->GetTriple();
        });
        shared_triples_0_->Precompute(1);
        std::tie(u0, v0, w0) = shared_triples_0_->GetTriple();
        thread1.join();

        EXPECT_EQ(u0.rows(), l);
        EXPECT_EQ(u0.cols(), m);
        EXPECT_EQ(v0.rows(), m);
        EXPECT_EQ(v0.cols(), n);
        EXPECT_EQ(u1.rows(), l);
        EXPECT_EQ(u1.cols(), m);
        EXPECT_EQ(v1.rows(), m);
        EXPECT_EQ(v1.cols(), n);
        EXPECT_EQ(w0.rows(), l);
        EXPECT_EQ(w1.rows(), l);
        EXPECT_EQ(w0.cols(), n);
        EXPECT_EQ(w1.cols(), n);
        EXPECT_EQ((u0 + u1) * (v0 + v1), w0 + w1);
      }
    }
  }
}

}  // namespace
}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra
