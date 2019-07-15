#include <random>
#include "sparse_linear_algebra/applications/knn/knn_protocol.hpp"
#include "sparse_linear_algebra/experiments/knn/knn_config.hpp"
#include "sparse_linear_algebra/util/get_ceil.hpp"
#include "sparse_linear_algebra/util/randomize_matrix.hpp"
#include "sparse_linear_algebra/util/reservoir_sampling.hpp"
#include "sparse_linear_algebra/util/time.h"

namespace sparse_linear_algebra {
namespace experiments {
namespace knn {

using sparse_linear_algebra::applications::knn::KNNProtocol;
using sparse_linear_algebra::applications::knn::MulType;
using sparse_linear_algebra::applications::knn::PirType;
using sparse_linear_algebra::experiments::knn::KNNConfig;

template <typename T>
void GenerateRandomMatrices(
    int seed, int precision, size_t num_documents_client, size_t num_words,
    size_t num_documents_server, size_t num_nonzeros_server,
    size_t num_nonzeros_client,
    Eigen::SparseMatrix<T, Eigen::RowMajor> *server_matrix,
    Eigen::SparseMatrix<T, Eigen::ColMajor> *client_matrix) {
  std::uniform_int_distribution<T> dist(0, 1000);

  std::mt19937 prg(seed);

  // Generate random triplets of the form (i, j, val) via a reservoir
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
  // Use the triplets to fill our matrices A and B (essentially,
  // `M[i][j] = val` is performed for each triplet `(i,j,val)`)
  // where M is A or B respectively.
  server_matrix->setFromTriplets(triplets_A.begin(), triplets_A.end());
  client_matrix->setFromTriplets(triplets_B.begin(), triplets_B.end());
}

void RunExperiments(comm_channel *channel, int party_id,
                    int16_t statistical_security, int precision,
                    const KNNConfig &conf) {
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
      mpc_utils::Benchmarker benchmarker;
      size_t chunk_size = get_ceil(conf.chunk_size, experiment);
      size_t k = get_ceil(conf.num_selected, experiment);
      size_t l = get_ceil(conf.num_documents, experiment);
      size_t m = get_ceil(conf.vocabulary_size, experiment);
      size_t n = 1;
      size_t num_nonzeros_server = get_ceil(conf.nonzeros_server, experiment);
      size_t num_nonzeros_client = get_ceil(conf.nonzeros_client, experiment);
      PirType pir_type = get_ceil(conf.pir_types, experiment);
      std::string pir_type_raw = get_ceil(conf.pir_types_raw, experiment);
      MulType multiplication_type =
          get_ceil(conf.multiplication_types, experiment);
      std::string multiplication_type_raw =
          get_ceil(conf.multiplication_types_raw, experiment);
      Eigen::SparseMatrix<T, Eigen::RowMajor> server_matrix(l, m);
      Eigen::SparseMatrix<T, Eigen::ColMajor> client_matrix(m, n);
      GenerateRandomMatrices<T>(123456, precision, l, m, n, num_nonzeros_server,
                                num_nonzeros_client, &server_matrix,
                                &client_matrix);

      std::cout << "num_documents_server = " << l << "\nm = " << m
                << "\nnum_documents_client = " << n
                << "\nnum_nonzeros_server = " << num_nonzeros_server
                << "\nnum_nonzeros_client = " << num_nonzeros_client
                << "\nchunk_size = " << chunk_size
                << "\npir_type = " << pir_type_raw
                << "\nmultiplication_type = " << multiplication_type_raw
                << "\n";

      KNNProtocol<T> protocol(channel, party_id, statistical_security,
                              precision, multiplication_type, pir_type,
                              chunk_size, k, l, m, n, num_nonzeros_server,
                              num_nonzeros_client, server_matrix,
                              client_matrix);
      auto result = protocol.run(&benchmarker);

      if (conf.measure_communication) {
        benchmarker.AddAmount("Bytes Sent (direct)",
                              channel->get_num_bytes_sent());
      }
      for (const auto &pair : benchmarker.GetAll()) {
        std::cout << pair.first << ": " << pair.second << "\n";
      }
      std::cout << "Top k documents: ";
      for (int i = 0; i < k; i++) {
        std::cout << result[i] << " ";
      }
    }
  }
}

}  // namespace knn
}  // namespace experiments
}  // namespace sparse_linear_algebra

// Generates random matrices and multiplies them using multiplication triples.
int main(int argc, const char *argv[]) {
  int precision = 10;

  sparse_linear_algebra::experiments::knn::KNNConfig conf;
  try {
    conf.parse(argc, argv);
  } catch (boost::program_options::error &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  // connect to other party
  party p(conf);
  auto channel = p.connect_to(1 - p.get_id(), conf.measure_communication);
  try {
    RunExperiments(&channel, p.get_id(), conf.statistical_security, precision,
                   conf);
  } catch (boost::exception &ex) {
    std::cerr << boost::diagnostic_information(ex);
    return 1;
  }
}
