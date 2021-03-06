#include "Eigen/Dense"
#include "Eigen/Sparse"
#include "mpc_utils/mpc_config.hpp"
#include "mpc_utils/party.hpp"
#include "sparse_linear_algebra/matrix_multiplication/cols-dense.hpp"
#include "sparse_linear_algebra/matrix_multiplication/cols-rows.hpp"
#include "sparse_linear_algebra/matrix_multiplication/dense.hpp"
#include "sparse_linear_algebra/matrix_multiplication/offline/fake_triple_provider.hpp"
#include "sparse_linear_algebra/matrix_multiplication/rows-dense.hpp"
#include "sparse_linear_algebra/oblivious_map/basic_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/poly_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/sorting_oblivious_map.hpp"
#include "sparse_linear_algebra/util/get_ceil.hpp"
#include "sparse_linear_algebra/util/randomize_matrix.hpp"
#include "sparse_linear_algebra/util/reservoir_sampling.hpp"
#include "sparse_linear_algebra/util/time.h"

class matrix_multiplication_config : public virtual mpc_config {
 protected:
  void validate() {
    namespace po = boost::program_options;
    if (party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
    }
    if (statistical_security <= 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'statistical_security' must be positive"));
    }
    if (rows_server.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'rows_server' must be passed at least once"));
    }
    for (auto l : rows_server) {
      if (l <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'rows_server' must be positive"));
      }
    }
    if (inner_dim.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'inner_dim' must be passed at least once"));
    }
    for (auto m : inner_dim) {
      if (m <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'inner_dim' must be positive"));
      }
    }
    if (cols_client.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'cols_client' must be passed at least once"));
    }
    for (auto n : cols_client) {
      if (n <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'cols_client' must be positive"));
      }
    }
    if (nonzeros_server.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'nonzeros_server' must be passed at least once"));
    }
    for (auto k_A : nonzeros_server) {
      if (k_A <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'nonzeros_server' must be positive"));
      }
    }
    if (nonzeros_client.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'nonzeros_client' must be passed at least once"));
    }
    for (auto k_B : nonzeros_client) {
      if (k_B <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'nonzeros_client' must be positive"));
      }
    }
    if (multiplication_types.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'multiplication_type' must be passed at least once"));
    }
    if (pir_types.size() == 0) {
      for (auto& mult_type : multiplication_types) {
        if (mult_type != "dense") {
          BOOST_THROW_EXCEPTION(
              po::error("'pir_type' must be passed at least once if "
                        "'multiplication_type'' is not \"dense\""));
        }
      }
    }
    for (auto& mult_type : multiplication_types) {
      if (mult_type != "dense" && mult_type != "cols_rows" &&
          mult_type != "rows_dense" && mult_type != "cols_dense") {
        BOOST_THROW_EXCEPTION(
            po::error("'multiplication_type' must be either "
                      "`cols_dense`, `rows_dense`, `cols_rows`, or `dense`"));
      }
    }
    for (auto& pir_type : pir_types) {
      if (pir_type != "basic" && pir_type != "poly" && pir_type != "scs") {
        BOOST_THROW_EXCEPTION(
            po::error("'pir_type' must be either "
                      "`basic`, `poly` or `scs`"));
      }
    }
    mpc_config::validate();
  }

 public:
  std::vector<ssize_t> rows_server;
  std::vector<ssize_t> inner_dim;
  std::vector<ssize_t> cols_client;
  std::vector<ssize_t> nonzeros_server;
  std::vector<ssize_t> nonzeros_client;
  std::vector<std::string> multiplication_types;
  std::vector<std::string> pir_types;
  int16_t statistical_security;
  ssize_t max_runs;
  bool skip_verification;
  bool measure_communication;

  matrix_multiplication_config() : mpc_config() {
    namespace po = boost::program_options;
    add_options()(
        "rows_server,l", po::value(&rows_server)->composing(),
        "Number of rows in the server's matrix; can be passed multiple times")(
        "inner_dim,m", po::value(&inner_dim)->composing(),
        "Number of columns in A and number of rows in B; can be passed "
        "multiple times")("cols_client,n", po::value(&cols_client)->composing(),
                          "Number of columns in the client's matrix; can be "
                          "passed multiple times")(
        "nonzeros_server,a", po::value(&nonzeros_server)->composing(),
        "Number of non-zero columns/rows in the server's matrix A; can be "
        "passed multiple times")(
        "nonzeros_client,b", po::value(&nonzeros_client)->composing(),
        "Number of non-zero rows/columns in the client's matrix B; can be "
        "passed multiple times")(
        "multiplication_type", po::value(&multiplication_types)->composing(),
        "Multiplication type: dense | cols_rows | cols_dense | rows_dense; can "
        "be passed multiple times")(
        "pir_type", po::value(&pir_types)->composing(),
        "PIR type: basic | poly | scs; can be passed multiple "
        "times")("statistical_security,s",
                 po::value(&statistical_security)->default_value(40),
                 "Statistical security parameter; used only for pir_type=poly")(
        "max_runs", po::value(&max_runs)->default_value(-1),
        "Maximum number of runs. Default is unlimited")(
        "skip_verification",
        po::bool_switch(&skip_verification)->default_value(false),
        "Skip verification")(
        "measure_communication",
        po::bool_switch(&measure_communication)->default_value(false),
        "Measure communication");
  }
};

// generates random matrices and multiplies them using multiplication triples
int main(int argc, const char* argv[]) {
  using sparse_linear_algebra::matrix_multiplication::offline::
      FakeTripleProvider;
  using T = uint64_t;
  // parse config
  matrix_multiplication_config conf;
  try {
    conf.parse(argc, argv);
  } catch (boost::program_options::error& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  // connect to other party
  party p(conf);
  auto channel = p.connect_to(1 - p.get_id(), conf.measure_communication);

  std::map<std::string, std::shared_ptr<oblivious_map<size_t, size_t>>>
      protos_perm{
          {"basic",
           std::make_shared<basic_oblivious_map<size_t, size_t>>(channel)},
          {"poly", std::make_shared<poly_oblivious_map<size_t, size_t>>(
                       channel, conf.statistical_security)},
          {"scs",
           std::make_shared<sorting_oblivious_map<size_t, size_t>>(channel)},
      };
  std::map<std::string, std::shared_ptr<oblivious_map<size_t, T>>> protos_val{
      {"basic", std::make_shared<basic_oblivious_map<size_t, T>>(channel)},
      {"poly", std::make_shared<poly_oblivious_map<size_t, T>>(
                   channel, conf.statistical_security)},
      {"scs", std::make_shared<sorting_oblivious_map<size_t, T>>(channel)},
  };
  using dense_matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  int seed = 12345;  // seed random number generator deterministically
  std::mt19937 prg(seed);
  std::uniform_int_distribution<T> dist;

  // generate test data
  size_t num_experiments = std::max(
      {conf.rows_server.size(), conf.inner_dim.size(), conf.cols_client.size(),
       conf.nonzeros_server.size(), conf.nonzeros_client.size(),
       conf.pir_types.size(), conf.multiplication_types.size()});
  for (size_t num_runs = 0; conf.max_runs < 0 || num_runs < conf.max_runs;
       num_runs++) {
    for (size_t experiment = 0; experiment < num_experiments; experiment++) {
      std::cout << "Run " << num_runs << "\n";
      mpc_utils::Benchmarker benchmarker;
      size_t l = get_ceil(conf.rows_server, experiment);
      size_t m = get_ceil(conf.inner_dim, experiment);
      size_t n = get_ceil(conf.cols_client, experiment);
      size_t k_A = get_ceil(conf.nonzeros_server, experiment);
      size_t k_B = get_ceil(conf.nonzeros_client, experiment);
      auto& type = get_ceil(conf.pir_types, experiment);
      auto& mult_type = get_ceil(conf.multiplication_types, experiment);
      std::cout << "l = " << l << "\nm = " << m << "\nn = " << n
                << "\nk_A = " << k_A << "\nk_B = " << k_B
                << "\npir_type = " << type
                << "\nmultiplication_type = " << mult_type << "\n";
      size_t& dense_rows = (mult_type == "rows_dense" ? k_A : l);
      ssize_t chunk_size = dense_rows;
      if (chunk_size > 4096) {
        // avoid memory errors
        chunk_size = 2048;
        // this will affect running times a little
        // TODO: make triplegenerator more general to support
        // triples for different chunk sizes
        if (dense_rows % chunk_size) {
          dense_rows = dense_rows + chunk_size - (dense_rows % chunk_size);
          std::cout << "Adjusting " << (mult_type == "rows_dense" ? "k_A" : "l")
                    << " to " << dense_rows << "\n";
        }
      }
      Eigen::SparseMatrix<T, Eigen::RowMajor> A(l, m);
      Eigen::SparseMatrix<T, Eigen::ColMajor> B(m, n);
      std::cout << "Generating random data\n";

      std::vector<Eigen::Triplet<T>> triplets_A;
      if (mult_type == "rows_dense") {
        auto indices_A = reservoir_sampling(prg, k_A, l);
        for (size_t i = 0; i < indices_A.size(); i++) {
          for (size_t j = 0; j < A.cols(); j++) {
            triplets_A.push_back(Eigen::Triplet<T>(indices_A[i], j, dist(prg)));
          }
        }
      } else {
        auto indices_A = reservoir_sampling(prg, k_A, m);
        for (size_t j = 0; j < indices_A.size(); j++) {
          for (size_t i = 0; i < A.rows(); i++) {
            triplets_A.push_back(Eigen::Triplet<T>(i, indices_A[j], dist(prg)));
          }
        }
      }
      A.setFromTriplets(triplets_A.begin(), triplets_A.end());
      auto indices_B = reservoir_sampling(prg, k_B, m);
      std::vector<Eigen::Triplet<T>> triplets_B;
      for (size_t i = 0; i < indices_B.size(); i++) {
        for (size_t j = 0; j < B.cols(); j++) {
          triplets_B.push_back(Eigen::Triplet<T>(indices_B[i], j, dist(prg)));
        }
      }
      B.setFromTriplets(triplets_B.begin(), triplets_B.end());

      try {
        dense_matrix C, C2;
        if (mult_type == "dense") {
          FakeTripleProvider<T> triples(chunk_size, m, n, p.get_id());
          channel.sync();
          benchmarker.BenchmarkFunction("Fake Triple Generation", [&] {
            triples.Precompute(l / chunk_size);
          });

          channel.sync();
          benchmarker.BenchmarkFunction("Matrix Multiplication", [&] {
            C = matrix_multiplication_dense(dense_matrix(A), dense_matrix(B),
                                            channel, p.get_id(), triples,
                                            chunk_size);
          });
        } else if (mult_type == "cols_rows") {
          FakeTripleProvider<T> triples(chunk_size, k_A + k_B, n, p.get_id());
          channel.sync();
          benchmarker.BenchmarkFunction("Fake Triple Generation", [&] {
            triples.Precompute(l / chunk_size);
          });

          channel.sync();
          benchmarker.BenchmarkFunction("Matrix Multiplication", [&] {
            C = matrix_multiplication_cols_rows(
                A, B, *protos_perm[type], channel, p.get_id(), triples,
                chunk_size, k_A, k_B, &benchmarker);
          });
        } else if (mult_type == "cols_dense") {
          FakeTripleProvider<T, true> triples(chunk_size, k_A, n, p.get_id());
          channel.sync();
          benchmarker.BenchmarkFunction("Fake Triple Generation", [&] {
            triples.Precompute(l / chunk_size);
          });

          channel.sync();
          benchmarker.BenchmarkFunction("Matrix Multiplication", [&] {
            C = matrix_multiplication_cols_dense(
                A, dense_matrix(B), *protos_val[type], channel, p.get_id(),
                triples, chunk_size, k_A, &benchmarker);
          });
        } else if (mult_type == "rows_dense") {
          FakeTripleProvider<T, false> triples(chunk_size, m, n, p.get_id());
          channel.sync();
          benchmarker.BenchmarkFunction("Fake Triple Generation", [&] {
            triples.Precompute(k_A / chunk_size);
          });

          channel.sync();
          benchmarker.BenchmarkFunction("Matrix Multiplication", [&] {
            C = matrix_multiplication_rows_dense(A, dense_matrix(B), channel,
                                                 p.get_id(), triples,
                                                 chunk_size, k_A, &benchmarker);
          });
        } else {
          BOOST_THROW_EXCEPTION(
              std::runtime_error("Unknown multiplication_type"));
        }

        // exchange shares for checking result
        if (!conf.skip_verification) {
          std::cout << "Verifying\n";
          channel.send_recv(C, C2);
          if (p.get_id() == 0) {
            channel.send_recv(A, B);
          } else {
            channel.send_recv(B, A);
          }

          // verify result
          C += C2;
          C2 = A * B;
          for (size_t i = 0; i < C.rows(); i++) {
            for (size_t j = 0; j < C.cols(); j++) {
              if (C(i, j) != C2(i, j)) {
                std::cerr << "Verification failed at index (" << i << ", " << j
                          << "). Expected " << C2(i, j) << ", got " << C(i, j)
                          << "\n";
              }
            }
          }
          std::cout << "Verification finished\n";
        } else {
          std::cout << "Skipping verification\n";
        }
      } catch (boost::exception& ex) {
        std::cerr << boost::diagnostic_information(ex);
        return 1;
      }

      if (conf.measure_communication) {
        benchmarker.AddAmount("Bytes Sent (direct)",
                              channel.get_num_bytes_sent());
      }
      for (const auto& pair : benchmarker.GetAll()) {
        std::cout << pair.first << ": " << pair.second << "\n";
      }
    }
  }
}
