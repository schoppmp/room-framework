#include <random>
#include "sparse_linear_algebra/applications/knn/knn_protocol.hpp"

namespace sparse_linear_algebra::util::knn {
  using T = uint64_t;
  using namespace sparse_linear_algebra::applications::knn;
  class KNNConfig : public virtual mpc_config {
  protected:
    void validate();
  public:
    std::map<std::string, PirType> pir_type_converison_table{
      {"basic", basic},
      {"poly", poly},
      {"scs", scs},
    };
    std::map<std::string, MulType> mul_type_converison_table{
      {"dense", dense},
      {"sparse", sparse},
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
  // Runs a number of experiments according to the provided `KNNProtocolConfig` and prints the
  // results.
  void runExperiments(comm_channel* channel, int party_id, int16_t statistical_security,int precision, KNNConfig& config);
  void generateRandomMatrices(int seed, int precision, size_t num_documents_client, size_t num_words, 
  size_t num_documents_server, size_t num_nonzeros_server, size_t num_nonzeros_client, Eigen::SparseMatrix<T, Eigen::RowMajor>* server_matrix,  Eigen::SparseMatrix<T, Eigen::ColMajor>* client_matrix);
}