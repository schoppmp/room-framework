#include "matrix_multiplication/dense.hpp"
#include "matrix_multiplication/sparse.hpp"
#include "mpc-utils/mpc_config.hpp"
#include "mpc-utils/party.hpp"
#include "util/randomize_matrix.hpp"
#include "util/reservoir_sampling.hpp"
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
    if(max_rows_server <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'rows_server' must be positive"));
    }
    if(inner_dim <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'inner_dim' must be positive"));
    }
    if(max_cols_client <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'cols_client' must be positive"));
    }
    if(max_nonzero_cols_server <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'nonzero_cols_server' must be positive"));
    }
    if(max_nonzero_rows_client <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'nonzero_rows_client' must be positive"));
    }
    if(statistical_security <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'statistical_security' must be positive"));
    }
    for(auto& pir_type : pir_types) {
      if(pir_type != "poly" && pir_type != "fss" && pir_type != "fss_cprg" && pir_type != "scs") {
      BOOST_THROW_EXCEPTION(po::error("'pir_type' must be either `poly`, `scs`, `fss` or `fss_cprg`"));
      }
    }
    mpc_config::validate();
  }
public:
  ssize_t max_rows_server;
  ssize_t inner_dim;
  ssize_t max_cols_client;
  ssize_t max_nonzero_cols_server;
  ssize_t max_nonzero_rows_client;
  int16_t statistical_security;
  std::vector<std::string> pir_types;

  matrix_multiplication_config() : mpc_config() {
    namespace po = boost::program_options;
    add_options()
      ("max_rows_server,l", po::value(&max_rows_server)->required(), "Number of rows in the server's matrix")
      ("inner_dim,m", po::value(&inner_dim)->required(), "Number of columns in A and number of rows in B")
      ("max_cols_client,n", po::value(&max_cols_client)->required(), "Number of columns in the client's matrix")
      ("max_nonzero_cols_server,k_A", po::value(&max_nonzero_cols_server)->required(), "Number of non-zero columns in the server's matrix A")
      ("max_nonzero_rows_client,k_B", po::value(&max_nonzero_rows_client)->required(), "Number of non-zero rows in the client's B")
      ("statistical_security,s", po::value(&statistical_security)->default_value(40), "Statistical security parameter; used only for pir_type=poly")
      ("pir_type", po::value(&pir_types)->composing(), "PIR type: poly | fss | scs; can be passed multiple times");
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
  size_t m = conf.inner_dim;
  for(size_t num_runs = 0; ; num_runs++) {
    std::cout << "Run " << num_runs << "\n";
    for(size_t l = 1; l <= conf.max_rows_server; l*=2) {                // I can
    for(size_t n = 1; n <= conf.max_cols_client; n*=2) {                // haz
    for(size_t k_A = 64; k_A <= conf.max_nonzero_cols_server; k_A*=2) { // Rust
    for(size_t k_B = 64; k_B <= conf.max_nonzero_rows_client; k_B*=2) { // PLLZZZ
      std::cout << "l = " << l << "\nn = " << n << "\nk_A = " << k_A << "\nk_B = "
        << k_B << "\n";
      ssize_t chunk_size = l;
      Eigen::SparseMatrix<T, Eigen::RowMajor> A(l, m);
      Eigen::SparseMatrix<T, Eigen::ColMajor> B(m, n);
      std::cout << "Generating random data\n";

      auto indices_A = reservoir_sampling(prg, k_A, m);
      auto indices_B =  reservoir_sampling(prg, k_B, m);
      std::vector<size_t> k_As(l, k_A), k_Bs(n, k_B);
      // reserve space explicitly instead of using triplets to make sure we are
      // independent of m
      A.reserve(k_As);
      B.reserve(k_Bs);
      for(size_t j = 0; j < indices_A.size(); j++) {
        for(size_t i = 0; i < A.rows(); i++) {
          A.insert(i, indices_A[j]) = dist(prg);
        }
      }
      for(size_t i = 0; i < indices_B.size(); i++) {
        for(size_t j = 0; j < B.cols(); j++) {
          B.insert(indices_B[i], j) = dist(prg);
        }
      }

      for(auto type : conf.pir_types) {
        std::cout << "Running sparse matrix multiplication (" << type << ")\n";
        try {
            fake_triple_provider<T> triples(chunk_size, k_A + k_B, n, p.get_id());
            channel.sync();
            benchmark([&]{
              triples.precompute(l / chunk_size);
            }, "Fake Triple Generation");

            dense_matrix C;
            channel.sync();
            benchmark([&]{
              C = matrix_multiplication(A, B, *protos[type],
                channel, p.get_id(), triples, chunk_size, k_A, k_B);
            }, "Sparse matrix multiplication");

            // exchange shares for checking result
            std::cout << "Verifying\n";
            dense_matrix C2;
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
    }}}}
  }
}
