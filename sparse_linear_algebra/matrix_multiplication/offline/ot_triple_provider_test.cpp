#include "sparse_linear_algebra/matrix_multiplication/offline/ot_triple_provider.hpp"
#include "absl/memory/memory.h"
#include "boost/thread.hpp"
#include "emp-ot/emp-ot.h"
#include "emp-tool/emp-tool.h"
#include "gtest/gtest.h"
#include "mpc_utils/boost_serialization/eigen.hpp"
#include "mpc_utils/comm_channel.hpp"

namespace sparse_linear_algebra {
namespace matrix_multiplication {
namespace offline {
namespace {

// TODO: Merge runParty and runParty2 into a single function

void runParty(int l, int m, int n, int role) {
  Matrix<uint64_t> u, v, w1, w2;
  mpc_config conf;
  conf.servers = {server_info("127.0.0.23", 1235),
                  server_info("127.0.0.42", 1235)};
  conf.party_id = role;
  party p(conf);
  emp::NetIO io(!role ? nullptr : "127.0.0.1", 1234);
  OTTripleProvider<uint64_t, false> triples(l, m, n, role, &io);
  triples.Precompute(1);
  auto channel = p.connect_to(!role ? 1 : 0);
  channel.sync();
  if (role == 0) {
    std::tie(u, std::ignore, w1) = triples.GetTriple();
    channel.recv(v);
    channel.recv(w2);
    EXPECT_EQ(u.rows(), l);
    EXPECT_EQ(u.cols(), m);
    EXPECT_EQ(v.rows(), m);
    EXPECT_EQ(v.cols(), n);
    EXPECT_EQ(w1.rows(), l);
    EXPECT_EQ(w2.rows(), l);

    EXPECT_EQ(w1.cols(), n);
    EXPECT_EQ(w2.cols(), n);
    EXPECT_EQ((u * v), (w1 + w2));
  } else {
    std::tie(std::ignore, v, w2) = triples.GetTriple();
    channel.send(v);
    channel.send(w2);
  }
}

void runParty2(int l, int m, int n, int role) {
  Matrix<uint64_t> u1, u2, v1, v2, w1, w2;
  mpc_config conf;
  conf.servers = {server_info("127.0.0.23", 1237),
                  server_info("127.0.0.42", 1237)};
  conf.party_id = role;
  party p(conf);
  emp::NetIO io(!role ? nullptr : "127.0.0.1", 1236);
  OTTripleProvider<uint64_t, true> triples(l, m, n, role, &io);
  triples.Precompute(1);
  auto channel = p.connect_to(!role ? 1 : 0);
  channel.sync();
  if (role == 0) {
    std::tie(u1, v1, w1) = triples.GetTriple();
    channel.recv(u2);
    channel.recv(v2);
    channel.recv(w2);
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
  } else {
    std::tie(u2, v2, w2) = triples.GetTriple();
    channel.send(u2);
    channel.send(v2);
    channel.send(w2);
  }
}

TEST(OTTripleProvider, DistributedTriple) {
  int l = 2, m = 2, n = 2;
  emp::initialize_relic();
  boost::thread thread1([this, l, m, n] { runParty(l, m, n, 0); });
  runParty(l, m, n, 1);
  boost::thread_guard<> g(thread1);
}

TEST(OTTripleProvider, SharedTriple) {
  int l = 128, m = 100, n = 1;
  emp::initialize_relic();
  boost::thread thread1([this, l, m, n] { runParty2(l, m, n, 0); });
  runParty2(l, m, n, 1);
  boost::thread_guard<> g(thread1);
}

}  // namespace
}  // namespace offline
}  // namespace matrix_multiplication
}  // namespace sparse_linear_algebra
