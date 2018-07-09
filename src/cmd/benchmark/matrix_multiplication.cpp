#include "matrix_multiplication/dense.hpp"
#include "matrix_multiplication/sparse.hpp"
#include "mpc-utils/mpc_config.hpp"
#include "mpc-utils/party.hpp"
#include "util/randomize_matrix.hpp"
#include "util/reservoir_sampling.hpp"
#include "util/get_ceil.hpp"
#include "util/time.h"
#include "pir_protocol.hpp"
#include "pir_protocol_scs.hpp"
#include "pir_protocol_fss.hpp"
#include "pir_protocol_poly.hpp"
#include <Eigen/Dense>
#include <Eigen/Sparse>

class matrix_multiplication_config : public virtual mpc_config {
protected:
  void validate() {
    namespace po = boost::program_options;
    if(party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
    }
    if(statistical_security <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'statistical_security' must be positive"));
    }
    for(auto l : rows_server) {
      if(l <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'rows_server' must be positive"));
      }
    }
    for(auto m : inner_dim) {
      if(m <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'inner_dim' must be positive"));
      }
    }
    for(auto n : cols_client) {
      if(n <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'cols_client' must be positive"));
      }
    }
    for(auto k_A : nonzero_cols_server) {
      if(k_A <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'nonzero_cols_server' must be positive"));
      }
    }
    for(auto k_B : nonzero_rows_client) {
      if(k_B <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'nonzero_rows_client' must be positive"));
      }
    }
    for(auto& pir_type : pir_types) {
      if(pir_type != "dense" && pir_type != "poly" && pir_type != "fss" && pir_type != "fss_cprg" && pir_type != "scs") {
      BOOST_THROW_EXCEPTION(po::error("'pir_type' must be either `dense`, `poly`, `scs`, `fss` or `fss_cprg`"));
      }
    }
    mpc_config::validate();
  }
public:
  std::vector<ssize_t> rows_server;
  std::vector<ssize_t> inner_dim;
  std::vector<ssize_t> cols_client;
  std::vector<ssize_t> nonzero_cols_server;
  std::vector<ssize_t> nonzero_rows_client;
  std::vector<std::string> pir_types;
  int16_t statistical_security;

  matrix_multiplication_config() : mpc_config() {
    namespace po = boost::program_options;
    add_options()
      ("rows_server,l", po::value(&rows_server)->composing(), "Number of rows in the server's matrix; can be passed multiple times")
      ("inner_dim,m", po::value(&inner_dim)->composing(), "Number of columns in A and number of rows in B; can be passed multiple times")
      ("cols_client,n", po::value(&cols_client)->composing(), "Number of columns in the client's matrix; can be passed multiple times")
      ("nonzero_cols_server,a", po::value(&nonzero_cols_server)->composing(), "Number of non-zero columns in the server's matrix A; can be passed multiple times")
      ("nonzero_rows_client,b", po::value(&nonzero_rows_client)->composing(), "Number of non-zero rows in the client's B; can be passed multiple times")
      ("pir_type", po::value(&pir_types)->composing(), "PIR type: dense | poly | fss | scs; can be passed multiple times")
      ("statistical_security,s", po::value(&statistical_security)->default_value(40), "Statistical security parameter; used only for pir_type=poly");
    set_default_filename("config/benchmark/matrix_multiplication.ini");
  }
};


// generates random matrices and multiplies them using multiplication triples
int main(int argc, const char *argv[]) {
  using T = uint32_t;
  // parse config
  matrix_multiplication_config conf;
  try {
    conf.parse(argc, argv);
  } catch (boost::program_options::error &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  // connect to other party
  party p(conf);
  auto channel = p.connect_to(1 - p.get_id());

  std::map<std::string, std::shared_ptr<pir_protocol<size_t, size_t>>> protos {
    {"poly", std::make_shared<pir_protocol_poly<size_t, size_t>>(channel, conf.statistical_security, true)},
    {"scs", std::make_shared<pir_protocol_scs<size_t, size_t>>(channel, true)},
    {"fss_cprg", std::make_shared<pir_protocol_fss<size_t, size_t>>(channel, true)},
    {"fss", std::make_shared<pir_protocol_fss<size_t, size_t>>(channel)},
  };
  using dense_matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  int seed = 12345; // seed random number generator deterministically
  std::mt19937 prg(seed);
  std::uniform_int_distribution<T> dist;

  // generate test data
  size_t num_experiments = std::max({conf.rows_server.size(),
    conf.inner_dim.size(), conf.cols_client.size(), conf.nonzero_cols_server.size(),
    conf.nonzero_rows_client.size(), conf.pir_types.size()});
  for(size_t num_runs = 0; ; num_runs++) {
    for(size_t experiment = 0; experiment < num_experiments; experiment++) {
      std::cout << "Run " << num_runs << "\n";
      size_t l = get_ceil(conf.rows_server, experiment);
      size_t m = get_ceil(conf.inner_dim, experiment);
      size_t n = get_ceil(conf.cols_client, experiment);
      size_t k_A = get_ceil(conf.nonzero_cols_server, experiment);
      size_t k_B = get_ceil(conf.nonzero_rows_client, experiment);
      auto& type = get_ceil(conf.pir_types, experiment);
      std::cout << "l = " << l << "\nn = " << n << "\nk_A = " << k_A << "\nk_B = "
        << k_B << "\npir_type = " << type << "\n";
      ssize_t chunk_size = l;
      Eigen::SparseMatrix<T, Eigen::RowMajor> A(l, m);
      Eigen::SparseMatrix<T, Eigen::ColMajor> B(m, n);
      std::cout << "Generating random data\n";

      auto indices_A = reservoir_sampling(prg, k_A, m);
      std::vector<Eigen::Triplet<T>> triplets_A;
      for(size_t j = 0; j < indices_A.size(); j++) {
        for(size_t i = 0; i < A.rows(); i++) {
          triplets_A.push_back(Eigen::Triplet<T>(i, indices_A[j], dist(prg)));
        }
      }
      A.setFromTriplets(triplets_A.begin(), triplets_A.end());
      auto indices_B =  reservoir_sampling(prg, k_B, m);
      std::vector<Eigen::Triplet<T>> triplets_B;
      for(size_t i = 0; i < indices_B.size(); i++) {
        for(size_t j = 0; j < B.cols(); j++) {
          triplets_B.push_back(Eigen::Triplet<T>(indices_B[i], j, dist(prg)));
        }
      }
      B.setFromTriplets(triplets_B.begin(), triplets_B.end());

      try {
        dense_matrix C, C2;
        if(type == "dense") {
          fake_triple_provider<T> triples(chunk_size, m, n, p.get_id());
          channel.sync();
          benchmark([&]{
            triples.precompute(l / chunk_size);
          }, "Fake Triple Generation");

          channel.sync();
          benchmark([&]{
            C = matrix_multiplication(dense_matrix(A), dense_matrix(B), channel,
              p.get_id(), triples, chunk_size);
          }, "Dense matrix multiplication");
        } else {
          fake_triple_provider<T> triples(chunk_size, k_A + k_B, n, p.get_id());
          channel.sync();
          benchmark([&]{
            triples.precompute(l / chunk_size);
          }, "Fake Triple Generation");

          channel.sync();
          benchmark([&]{
            C = matrix_multiplication(A, B, *protos[type],
              channel, p.get_id(), triples, chunk_size, k_A, k_B);
          }, "Sparse matrix multiplication");
        }

        // exchange shares for checking result
        std::cout << "Verifying\n";
        channel.send_recv(C, C2);
        if(p.get_id() == 0) {
            channel.send_recv(A, B);
        } else {
            channel.send_recv(B, A);
        }

        // verify result
        C += C2;
        C2 = A * B;
        for(size_t i = 0; i < C.rows(); i++) {
          for(size_t j = 0; j < C.cols(); j++) {
            if(C(i, j) != C2(i, j)) {
              std::cerr << "Verification failed at index (" << i << ", " << j
                        << "). Expected " << C2(i, j) << ", got " << C(i, j) <<"\n";
            }
          }
        }
        std::cout << "Verification finished\n";
      } catch(boost::exception &ex) {
        std::cerr << boost::diagnostic_information(ex);
        return 1;
      }
    }
  }
}
