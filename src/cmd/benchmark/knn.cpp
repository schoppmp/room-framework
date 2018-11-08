extern "C" {
  #include "applications/knn.h"
}
#include "matrix_multiplication/dense.hpp"
#include "matrix_multiplication/sparse/cols-rows.hpp"
#include "matrix_multiplication/sparse/cols-dense.hpp"
#include "matrix_multiplication/sparse/rows-dense.hpp"
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
#include "pir_protocol_basic.hpp"
#include <Eigen/Dense>
#include <Eigen/Sparse>

class knn_config : public virtual mpc_config {
protected:
  void validate() {
    namespace po = boost::program_options;
    if(party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
    }
    if(statistical_security <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'statistical_security' must be positive"));
    }
    if(num_documents.size() == 0) {
      BOOST_THROW_EXCEPTION(po::error("'num_documents' must be passed at least once"));
    }
    for(auto l : num_documents) {
      if(l <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'num_documents' must be positive"));
      }
    }
    if(num_selected.size() == 0) {
      BOOST_THROW_EXCEPTION(po::error("'num_selected' must be passed at least once"));
    }
    for(auto k : num_selected) {
      if(k <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'num_selected' must be positive"));
      }
    }
    if(chunk_size.size() == 0) {
      BOOST_THROW_EXCEPTION(po::error("'chunk_size' must be passed at least once"));
    }
    for(auto c : chunk_size) {
      if(c <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'chunk_size' must be positive"));
      }
    }
    if(vocabulary_size.size() == 0) {
      BOOST_THROW_EXCEPTION(po::error("'vocabulary_size' must be passed at least once"));
    }
    for(auto m : vocabulary_size) {
      if(m <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'vocabulary_size' must be positive"));
      }
    }
    if(nonzeros_server.size() == 0) {
      BOOST_THROW_EXCEPTION(po::error("'nonzeros_server' must be passed at least once"));
    }
    for(auto k_A : nonzeros_server) {
      if(k_A <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'nonzeros_server' must be positive"));
      }
    }
    if(nonzeros_client.size() == 0) {
      BOOST_THROW_EXCEPTION(po::error("'nonzeros_client' must be passed at least once"));
    }
    for(auto k_B : nonzeros_client) {
      if(k_B <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'nonzeros_client' must be positive"));
      }
    }
    if(multiplication_types.size() == 0) {
      BOOST_THROW_EXCEPTION(po::error("'multiplication_type' must be passed at least once"));
    }
    if(pir_types.size() == 0) {
      for(auto& mult_type : multiplication_types) {
        if(mult_type != "dense") {
          BOOST_THROW_EXCEPTION(po::error("'pir_type' must be passed at least once if 'multiplication_type'' is not \"dense\""));
        }
      }
    }
    for(auto& mult_type : multiplication_types) {
      if(
        mult_type != "dense" &&
        mult_type != "sparse"
      ) {
        BOOST_THROW_EXCEPTION(po::error("'multiplication_type' must be either "
          "`sparse`, or `dense`"));
      }
    }
    for(auto& pir_type : pir_types) {
      if(
        pir_type != "basic" &&
        pir_type != "poly" &&
        pir_type != "scs"
      ) {
        BOOST_THROW_EXCEPTION(po::error("'pir_type' must be either "
          "`basic`, `poly`, or `scs`"));
      }
    }
    mpc_config::validate();
  }
public:
  std::vector<ssize_t> num_documents;
  std::vector<ssize_t> num_selected;
  std::vector<ssize_t> chunk_size;
  std::vector<ssize_t> vocabulary_size;
  std::vector<ssize_t> nonzeros_server;
  std::vector<ssize_t> nonzeros_client;
  std::vector<std::string> multiplication_types;
  std::vector<std::string> pir_types;
  int16_t statistical_security;
  ssize_t max_runs;
  // bool measure_communication;

  knn_config() : mpc_config() {
    namespace po = boost::program_options;
    add_options()
      ("num_documents,l", po::value(&num_documents)->composing(), "Number of rows in the server's matrix; can be passed multiple times")
      ("num_selected,k", po::value(&num_selected)->composing(), "Number of documents to be returned; i.e, the k in k-NN. Can be passed multiple times")
      ("chunk_size", po::value(&chunk_size)->composing(), "Number of documents to process at once; can be passed multiple times")
      ("vocabulary_size,m", po::value(&vocabulary_size)->composing(), "Number of columns in A and number of rows in B; can be passed multiple times")
      ("nonzeros_server,a", po::value(&nonzeros_server)->composing(), "Number of non-zero columns in the server's matrix A; can be passed multiple times")
      ("nonzeros_client,b", po::value(&nonzeros_client)->composing(), "Number of non-zero elements in the client's vector b; can be passed multiple times")
      ("multiplication_type", po::value(&multiplication_types)->composing(), "Multiplication type: dense | sparse; can be passed multiple times")
      ("pir_type", po::value(&pir_types)->composing(), "PIR type: basic | poly | scs; can be passed multiple times")
      ("statistical_security,s", po::value(&statistical_security)->default_value(40), "Statistical security parameter; used only for pir_type=poly")
      ("max_runs", po::value(&max_runs)->default_value(-1), "Maximum number of runs. Default is unlimited");
      // TODO:
      // ("measure_communication", po::bool_switch(&measure_communication)->default_value(false), "Measure communication");
    set_default_filename("config/benchmark/knn.ini");
  }
};

// generates random matrices and multiplies them using multiplication triples
int main(int argc, const char *argv[]) {
  using T = uint64_t;
  int precision = 10;
  // parse config
  knn_config conf;
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
    {"basic", std::make_shared<pir_protocol_basic<size_t, size_t>>(channel, true)},
    {"poly", std::make_shared<pir_protocol_poly<size_t, size_t>>(channel, conf.statistical_security, true)},
    {"scs", std::make_shared<pir_protocol_scs<size_t, size_t>>(channel, true)},
  };
  using dense_matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  int seed = 123456; // seed random number generator deterministically
  std::mt19937 prg(seed);
  std::uniform_int_distribution<T> dist(0, 1000);

  // generate test data
  size_t num_experiments = std::max({conf.num_documents.size(),
    conf.num_selected.size(), conf.chunk_size.size(),
    conf.vocabulary_size.size(), conf.nonzeros_server.size(),
    conf.nonzeros_client.size(), conf.pir_types.size(),
    conf.multiplication_types.size()});
  size_t n = 1;
  for (size_t num_runs = 0; conf.max_runs < 0 || num_runs < conf.max_runs; num_runs++) {
    for (size_t experiment = 0; experiment < num_experiments; experiment++) {
      std::cout << "Run " << num_runs << "\n";
      size_t chunk_size = get_ceil(conf.chunk_size, experiment);
      size_t k = get_ceil(conf.num_selected, experiment);
      size_t l = get_ceil(conf.num_documents, experiment);
      size_t m = get_ceil(conf.vocabulary_size, experiment);
      size_t k_A = get_ceil(conf.nonzeros_server, experiment);
      size_t k_B = get_ceil(conf.nonzeros_client, experiment);
      std::string& type = get_ceil(conf.pir_types, experiment);
      std::string& mult_type = get_ceil(conf.multiplication_types, experiment);
      std::cout << "l = " << l << "\nm = " << m << "\nn = " << n << "\nk_A = "
        << k_A << "\nk_B = " << k_B << "\nchunk_size = " << chunk_size
        << "\npir_type = " << type
        << "\nmultiplication_type = " << mult_type << "\n";
      Eigen::SparseMatrix<T, Eigen::RowMajor> A(l, m);
      Eigen::SparseMatrix<T, Eigen::ColMajor> B(m, n);
      std::cout << "Generating random data\n";

      std::vector<Eigen::Triplet<T>> triplets_A;
      std::vector<Eigen::Triplet<T>> triplets_B;
      auto indices_A = reservoir_sampling(prg, k_A, m);
      for(size_t j = 0; j < indices_A.size(); j++) {
        for(size_t i = 0; i < A.rows(); i++) {
          Eigen::Triplet<T> triplet(i, indices_A[j],
            dist(prg) * (static_cast<T>(1) << precision));
          triplets_A.push_back(triplet);
          if(i == 23 && j < k_B) {
            // Set B to the first k_B nonzero elements of the 23rd document.
            // Used as a correctness check.
            triplets_B.push_back(Eigen::Triplet<T>(indices_A[j], 0,
              triplet.value()));
          }
        }
      }
      A.setFromTriplets(triplets_A.begin(), triplets_A.end());
      // auto indices_B =  reservoir_sampling(prg, k_B, m);
      // for(size_t i = 0; i < indices_B.size(); i++) {
      //   for(size_t j = 0; j < B.cols(); j++) {
      //     triplets_B.push_back(Eigen::Triplet<T>(indices_B[i], j, dist(prg)));
      //   }
      // }
      B.setFromTriplets(triplets_B.begin(), triplets_B.end());

      // Compute similarities, chunk_size documents at a time.
      dense_matrix C(l, n);
      for (int row = 0; row < l; row += chunk_size) {
        // The last chunk might be smaller.
        int this_chunk_size = chunk_size;
        if(row + this_chunk_size > l) {
          this_chunk_size = l - row;
        }
        // Avoid out-of-memory errors by using a different chunk size for dense
        // matrix multiplication.
        int dense_chunk_size = this_chunk_size;
        if(dense_chunk_size > 4096) {
          dense_chunk_size = 2048;
        }
        std::cout << "this_chunk_size = " << this_chunk_size << "\n";
        std::cout << "dense_chunk_size = " << dense_chunk_size << "\n";
        try {
          if(mult_type == "dense") {
            fake_triple_provider<T> triples(dense_chunk_size, m, n, p.get_id());
            channel.sync();
            // TODO: aggregate fake triple computation times
            benchmark([&]{
              triples.precompute((this_chunk_size + dense_chunk_size - 1) /
                dense_chunk_size);
            }, "Fake Triple Generation");

            channel.sync();
            benchmark([&]{
              C.middleRows(row, this_chunk_size) = matrix_multiplication(
                dense_matrix(A).middleRows(row, this_chunk_size),
                dense_matrix(B), channel,
                p.get_id(), triples, dense_chunk_size);
            }, "Total");
          } else if(mult_type == "sparse") {
            fake_triple_provider<T> triples(dense_chunk_size, k_A + k_B, n,
              p.get_id());
            channel.sync();
            benchmark([&]{
              triples.precompute((this_chunk_size + dense_chunk_size - 1) /
                dense_chunk_size);
            }, "Fake Triple Generation");

            channel.sync();
            benchmark([&]{
              C.middleRows(row, this_chunk_size) = matrix_multiplication_cols_rows(
                A.middleRows(row, this_chunk_size), B, *protos[type],
                channel, p.get_id(), triples, dense_chunk_size, k_A, k_B, true);
            }, "Total");
          } else {
            BOOST_THROW_EXCEPTION(std::runtime_error("Unknown multiplication_type"));
          }
        } catch(boost::exception &ex) {
          std::cerr << boost::diagnostic_information(ex);
          return 1;
        }
      }

      // // Rescaling is not needed since we don't have nested multiplications.
      // for (int i = 0; i < l; ++i) {
      //   for (int j = 0; j < n; ++j) {
      //     C(i, j) = C(i, j) / (static_cast<T>(1) << precision);
      //   }
      // }

      // compute norms
      std::vector<T> norms_A(l);
      for(int i = 0; i < l; i++) {
        norms_A[i] = (A.row(i) / (static_cast<T>(1) << precision)).norm();
      }
      T norm_B = (B / (static_cast<T>(1) << precision)).norm();

      // run top-k selection
      ProtocolDesc pd;
      if(channel.connect_to_oblivc(pd) == -1) {
        BOOST_THROW_EXCEPTION(std::runtime_error(
          "run_server: connection failed"));
      }
      setCurrentParty(&pd, 1 + p.get_id());
      std::vector<uint8_t> serialized_inputs(l * sizeof(T));
      std::vector<uint8_t> serialized_norms;
      std::vector<uint8_t> serialized_outputs(k * sizeof(int));
      std::vector<T> inputs_vec(m);
      std::vector<int> outputs(k);
      for(int i = 0; i < l; i++) {
        inputs_vec[i] = C(i, 0);
      }
      serialize_le(serialized_inputs.begin(), inputs_vec.begin(), l);
      if(p.get_id() == 0) {
        serialized_norms.resize(l * sizeof(T));
        serialize_le(serialized_norms.begin(), norms_A.begin(), l);
      } else {
        serialized_norms.resize(sizeof(T));
        serialize_le(serialized_norms.begin(), &norm_B, 1);
      }
      knn_oblivc_args args = {
        static_cast<int>(l), static_cast<int>(k), sizeof(T), serialized_inputs.data(), serialized_norms.data(),
        outputs.data()
      };
      execYaoProtocol(&pd, knn_oblivc, &args);
      cleanupProtocol(&pd);

      // check result
      std::cout << "Top k documents: ";
      for (int i = 0; i < k; i++) {
        std::cout << outputs[i] << " ";
      }
      std::cout << "\n";

    }
  }
}
