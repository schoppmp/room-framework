#include "sparse_linear_algebra/matrix_multiplication/offline/ot_triple_provider.hpp"
#include "mpc_utils/testing/comm_channel_test_helper.hpp"
#include "gtest/gtest.h"
#include <boost/thread.hpp>

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {
namespace {

using mpc_utils::testing::CommChannelTestHelper;


TEST(OTTripleProvider, DistributedTriple) {
  int l = 2, m = 2, n = 2;
  CommChannelTestHelper helper(false);
  OTTripleProvider<uint64_t, false> triples1(l, m, n, 0, helper.GetChannel(0));
  OTTripleProvider<uint64_t, false> triples2(l, m, n, 1, helper.GetChannel(1));
  boost::thread thread1([this, &triples1] {
    triples1.Precompute(1);
  });
  boost::thread_guard<> g(thread1);
  triples2.Precompute(1);
  Matrix<uint64_t> u, v, w1, w2;
  std::tie(u, std::ignore, w1) = triples1.GetTriple();
  std::tie(std::ignore, v, w2) = triples2.GetTriple();
  EXPECT_EQ(u.rows(), l);
  EXPECT_EQ(u.cols(), m);
  EXPECT_EQ(v.rows(), m);
  EXPECT_EQ(v.cols(), n);
  EXPECT_EQ(w1.rows(), l);
  EXPECT_EQ(w2.rows(), l);
  EXPECT_EQ(w1.cols(), n);
  EXPECT_EQ(w2.cols(), n);
  EXPECT_EQ((u * v), (w1 + w2));
}

TEST(OTTripleProvider, SharedTriple) {
  int l = 20, m = 30, n = 40;
  CommChannelTestHelper helper(false);
  OTTripleProvider<uint64_t, true> triples1(l, m, n, 0, helper.GetChannel(0));
  OTTripleProvider<uint64_t, true> triples2(l, m, n, 1, helper.GetChannel(1));
  boost::thread thread1([this, &triples1] {
    triples1.Precompute(1);
  });
  boost::thread_guard<> g(thread1);
  triples2.Precompute(1);
  Matrix<uint64_t> u1, u2, v1, v2, w1, w2;
  std::tie(u1, v1, w1) = triples1.GetTriple();
  std::tie(u2, v2, w2) = triples2.GetTriple();
  EXPECT_EQ(u1.rows(), l);
  EXPECT_EQ(u2.rows(), l);
  EXPECT_EQ(u1.cols(), m);
  EXPECT_EQ(u2.cols(), m);
  EXPECT_EQ(v1.rows(), m);
  EXPECT_EQ(v2.rows(), m);
  EXPECT_EQ(v1.cols(), n);
  EXPECT_EQ(v2.cols(), n);
  EXPECT_EQ(w1.rows(), l);
  EXPECT_EQ(w2.rows(), l);
  EXPECT_EQ(w1.cols(), n);
  EXPECT_EQ(w2.cols(), n);
  EXPECT_EQ((u1 + u2) * (v1 + v2), w1 + w2);
}

}  // namespace
}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra
