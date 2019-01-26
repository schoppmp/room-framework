#include <random>
#include "sparse_linear_algebra/applications/knn/knn_protocol.hpp"
#include "sparse_linear_algebra/util/get_ceil.hpp"
#include "sparse_linear_algebra/util/randomize_matrix.hpp"
#include "sparse_linear_algebra/util/reservoir_sampling.hpp"
#include "sparse_linear_algebra/util/time.h"

namespace {
using sparse_linear_algebra::applications::knn::KNNProtocol;
using sparse_linear_algebra::applications::knn::MulType;
using sparse_linear_algebra::applications::knn::PirType;

class KNNConfig : public virtual mpc_config {
 protected:
  void validate();

 public:
  std::map<std::string, PirType> pir_type_converison_table{
      {"basic", PirType::basic},
      {"poly", PirType::poly},
      {"scs", PirType::scs},
  };
  std::map<std::string, MulType> mul_type_converison_table{
      {"dense", MulType::dense},
      {"sparse", MulType::sparse},
  };

  std::vector<ssize_t> num_documents;
  std::vector<ssize_t> num_selected;
  std::vector<ssize_t> chunk_size;
  std::vector<ssize_t> vocabulary_size;
  std::vector<ssize_t> nonzeros_server;
  std::vector<ssize_t> nonzeros_client;
  std::vector<std::string> multiplication_types_raw;
  std::vector<std::string> pir_types_raw;
  std::vector<MulType> multiplication_types;
  std::vector<PirType> pir_types;

  int16_t statistical_security;
  ssize_t max_runs;
  KNNConfig();
  // bool measure_communication;
};

void KNNConfig::validate() {
  namespace po = boost::program_options;
  if (party_id < 0 || party_id > 1) {
    BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
  }
  if (statistical_security <= 0) {
    BOOST_THROW_EXCEPTION(po::error("'statistical_security' must be positive"));
  }
  if (num_documents.size() == 0) {
    BOOST_THROW_EXCEPTION(
        po::error("'num_documents' must be passed at least once"));
  }
  for (auto l : num_documents) {
    if (l <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'num_documents' must be positive"));
    }
  }
  if (num_selected.size() == 0) {
    BOOST_THROW_EXCEPTION(
        po::error("'num_selected' must be passed at least once"));
  }
  for (auto k : num_selected) {
    if (k <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'num_selected' must be positive"));
    }
  }
  if (chunk_size.size() == 0) {
    BOOST_THROW_EXCEPTION(
        po::error("'chunk_size' must be passed at least once"));
  }
  for (auto c : chunk_size) {
    if (c <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'chunk_size' must be positive"));
    }
  }
  if (vocabulary_size.size() == 0) {
    BOOST_THROW_EXCEPTION(
        po::error("'vocabulary_size' must be passed at least once"));
  }
  for (auto m : vocabulary_size) {
    if (m <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'vocabulary_size' must be positive"));
    }
  }
  if (nonzeros_server.size() == 0) {
    BOOST_THROW_EXCEPTION(
        po::error("'nonzeros_server' must be passed at least once"));
  }
  for (auto num_nonzeros_server : nonzeros_server) {
    if (num_nonzeros_server <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'nonzeros_server' must be positive"));
    }
  }
  if (nonzeros_client.size() == 0) {
    BOOST_THROW_EXCEPTION(
        po::error("'nonzeros_client' must be passed at least once"));
  }
  for (auto num_nonzeros_client : nonzeros_client) {
    if (num_nonzeros_client <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'nonzeros_client' must be positive"));
    }
  }
  if (multiplication_types_raw.size() == 0) {
    BOOST_THROW_EXCEPTION(
        po::error("'multiplication_type' must be passed at least once"));
  }
  if (pir_types_raw.size() == 0) {
    for (auto& mult_type : multiplication_types_raw) {
      if (mult_type != "dense") {
        BOOST_THROW_EXCEPTION(
            po::error("'pir_type' must be passed at least once if "
                      "'multiplication_type'' is not \"dense\""));
      }
    }
  }
  for (auto& mult_type : multiplication_types_raw) {
    try {
      multiplication_types.push_back(mul_type_converison_table.at(mult_type));
    } catch (const std::out_of_range& e) {
      BOOST_THROW_EXCEPTION(
          po::error("'multiplication_type' must be either "
                    "`sparse`, or `dense`"));
    }
  }
  for (auto& pir_type : pir_types_raw) {
    try {
      pir_types.push_back(pir_type_converison_table.at(pir_type));
    } catch (const std::out_of_range& e) {
      BOOST_THROW_EXCEPTION(
          po::error("'pir_type' must be either "
                    "`basic`, `poly`, or `scs`"));
    }
  }
  mpc_config::validate();
}

KNNConfig::KNNConfig() : mpc_config() {
  namespace po = boost::program_options;
  add_options()(
      "num_documents,l", po::value(&num_documents)->composing(),
      "Number of rows in the server's matrix; can be passed multiple times")(
      "num_selected,k", po::value(&num_selected)->composing(),
      "Number of documents to be returned; i.e, the k in k-NN. Can be passed "
      "multiple times")(
      "chunk_size", po::value(&chunk_size)->composing(),
      "Number of documents to process at once; can be passed multiple times")(
      "vocabulary_size,m", po::value(&vocabulary_size)->composing(),
      "Number of columns in A and number of rows in B; can be passed "
      "multiple times")(
      "nonzeros_server,a", po::value(&nonzeros_server)->composing(),
      "Maximum number of non-zero columns in each chunk of the server's "
      "matrix A; can be passed multiple times")(
      "nonzeros_client,b", po::value(&nonzeros_client)->composing(),
      "Number of non-zero elements in the client's vector b; can be passed "
      "multiple times")(
      "multiplication_type", po::value(&multiplication_types_raw)->composing(),
      "Multiplication type: dense | sparse; can be passed multiple times")(
      "pir_type", po::value(&pir_types_raw)->composing(),
      "PIR type: basic | poly | scs; can be passed multiple times")(
      "statistical_security,s",
      po::value(&statistical_security)->default_value(40),
      "Statistical security parameter; used only for pir_type=poly")(
      "max_runs", po::value(&max_runs)->default_value(-1),
      "Maximum number of runs. Default is unlimited");
  // TODO:
  // ("measure_communication",
  // po::bool_switch(&measure_communication)->default_value(false), "Measure
  // communication");
  set_default_filename("sparse_linear_algebra/experiments/knn/knn.ini");
}

template <typename T>
void generateRandomMatrices(
    int seed, int precision, size_t num_documents_client, size_t num_words,
    size_t num_documents_server, size_t num_nonzeros_server,
    size_t num_nonzeros_client,
    Eigen::SparseMatrix<T, Eigen::RowMajor>* server_matrix,
    Eigen::SparseMatrix<T, Eigen::ColMajor>* client_matrix) {
  std::uniform_int_distribution<T> dist(0, 1000);

  std::mt19937 prg(seed);

  // generate random triplets of the form (i, j, val) via a reservoir
  // sampling.
  std::vector<Eigen::Triplet<T>> triplets_A;
  std::vector<Eigen::Triplet<T>> triplets_B;
  auto indices_A = reservoir_sampling(prg, num_nonzeros_server, num_words);
  for (size_t j = 0; j < indices_A.size(); j++) {
    for (size_t i = 0; i < server_matrix->rows(); i++) {
      Eigen::Triplet<T> triplet(i, indices_A[j],
                                dist(prg) * (static_cast<T>(1) << precision));
      triplets_A.push_back(triplet);
      if (i == 23 && j < num_nonzeros_client) {
        // Set B to the first num_nonzeros_client nonzero elements of the 23rd
        // document. Used as a correctness check.
        triplets_B.push_back(
            Eigen::Triplet<T>(indices_A[j], 0, triplet.value()));
      }
    }
  }
  // use the triplets to fill our matrices A and B (essentially,
  // `M[i][j] = val` is performed for each triplet `(i,j,val)`)
  // where M is A or B respectively
  server_matrix->setFromTriplets(triplets_A.begin(), triplets_A.end());
  client_matrix->setFromTriplets(triplets_B.begin(), triplets_B.end());
}

void runExperiments(comm_channel* channel, int party_id,
                    int16_t statistical_security, int precision,
                    const KNNConfig& conf) {
  using T = uint64_t;
  size_t num_experiments =
      std::max({conf.num_documents.size(), conf.num_selected.size(),
                conf.chunk_size.size(), conf.vocabulary_size.size(),
                conf.nonzeros_server.size(), conf.nonzeros_client.size(),
                conf.pir_types.size(), conf.multiplication_types.size()});
  for (size_t num_runs = 0; conf.max_runs < 0 || num_runs < conf.max_runs;
       num_runs++) {
    for (size_t experiment = 0; experiment < num_experiments; experiment++) {
      std::cout << "Run " << num_runs << "\n";
      size_t chunk_size = get_ceil(conf.chunk_size, experiment);
      size_t k = get_ceil(conf.num_selected, experiment);
      size_t l = get_ceil(conf.num_documents, experiment);
      size_t m = get_ceil(conf.vocabulary_size, experiment);
      size_t n = 1;
      size_t num_nonzeros_server = get_ceil(conf.nonzeros_server, experiment);
      size_t num_nonzeros_client = get_ceil(conf.nonzeros_client, experiment);
      PirType pt = get_ceil(conf.pir_types, experiment);
      MulType mt = get_ceil(conf.multiplication_types, experiment);
      Eigen::SparseMatrix<T, Eigen::RowMajor> server_matrix(l, m);
      Eigen::SparseMatrix<T, Eigen::ColMajor> client_matrix(m, n);
      generateRandomMatrices<T>(123456, precision, l, m, n, num_nonzeros_server,
                                num_nonzeros_client, &server_matrix,
                                &client_matrix);
      KNNProtocol<T> protocol(channel, party_id, statistical_security,
                              precision, mt, pt, chunk_size, k, l, m, n,
                              num_nonzeros_server, num_nonzeros_client,
                              server_matrix, client_matrix);
      auto result = protocol.run();
      std::cout << "Top k documents: ";
      for (int i = 0; i < k; i++) {
        std::cout << result[i] << " ";
      }
    }
  }
}

}  // namespace

// generates random matrices and multiplies them using multiplication triples
int main(int argc, const char* argv[]) {
  int precision = 10;

  KNNConfig conf;
  try {
    conf.parse(argc, argv);
  } catch (boost::program_options::error& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  // connect to other party
  party p(conf);
  auto channel = p.connect_to(1 - p.get_id());
  try {
    runExperiments(&channel, p.get_id(), conf.statistical_security, precision,
                   conf);
  } catch (boost::exception& ex) {
    std::cerr << boost::diagnostic_information(ex);
    return 1;
  }
}
