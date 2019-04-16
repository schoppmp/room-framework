#include "knn_config.hpp"
namespace sparse_linear_algebra {
namespace experiments {
namespace knn {
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
    for (auto &mult_type : multiplication_types_raw) {
      if (mult_type != "dense") {
        BOOST_THROW_EXCEPTION(
            po::error("'pir_type' must be passed at least once if "
                      "'multiplication_type'' is not \"dense\""));
      }
    }
  }
  for (auto &mult_type : multiplication_types_raw) {
    try {
      multiplication_types.push_back(mul_type_converison_table.at(mult_type));
    } catch (const std::out_of_range &e) {
      BOOST_THROW_EXCEPTION(
          po::error("'multiplication_type' must be either "
                    "`sparse`, or `dense`"));
    }
  }
  for (auto &pir_type : pir_types_raw) {
    try {
      pir_types.push_back(pir_type_converison_table.at(pir_type));
    } catch (const std::out_of_range &e) {
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
      "Maximum number of runs. Default is unlimited")(
      "measure_communication",
      po::bool_switch(&measure_communication)->default_value(false),
      "Measure communication");
}
}  // namespace knn
}  // namespace experiments
}  // namespace sparse_linear_algebra