#include "Eigen/Dense"
#include "Eigen/Sparse"
#include "mpc_utils/mpc_config.hpp"
#include "mpc_utils/party.hpp"
#include "sparse_linear_algebra/matrix_multiplication/cols-dense.hpp"
#include "sparse_linear_algebra/matrix_multiplication/cols-rows.hpp"
#include "sparse_linear_algebra/matrix_multiplication/dense.hpp"
#include "sparse_linear_algebra/matrix_multiplication/rows-dense.hpp"
#include "sparse_linear_algebra/oblivious_map/basic_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/poly_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/sorting_oblivious_map.hpp"
#include "sparse_linear_algebra/util/get_ceil.hpp"
#include "sparse_linear_algebra/util/randomize_matrix.hpp"
#include "sparse_linear_algebra/util/reservoir_sampling.hpp"
#include "sparse_linear_algebra/util/time.h"
extern "C" {
#include "sparse_linear_algebra/applications/sgd/sigmoid.h"
}

class sgd_config : public virtual mpc_config {
 protected:
  void validate() {
    namespace po = boost::program_options;
    if (party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
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
    if (batch_size.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'batch_size' must be passed at least once"));
    }
    for (auto c : batch_size) {
      if (c <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'batch_size' must be positive"));
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
    if (nonzeros_a.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'nonzeros_a' must be passed at least once"));
    }
    for (auto k_A : nonzeros_a) {
      if (k_A <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'nonzeros_a' must be positive"));
      }
    }
    if (nonzeros_b.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'nonzeros_b' must be passed at least once"));
    }
    for (auto k_B : nonzeros_b) {
      if (k_B <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'nonzeros_b' must be positive"));
      }
    }
    if (num_epochs.size() == 0) {
      num_epochs = {1};
    }
    for (auto e : num_epochs) {
      if (e <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'num_epochs' must be positive"));
      }
    }
    if (multiplication_types.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'multiplication_type' must be passed at least once"));
    }
    for (auto& mult_type : multiplication_types) {
      if (mult_type != "dense" && mult_type != "sparse") {
        BOOST_THROW_EXCEPTION(
            po::error("'multiplication_type' must be either "
                      "`sparse`, or `dense`"));
      }
    }
    mpc_config::validate();
  }

 public:
  std::vector<ssize_t> num_documents;
  std::vector<ssize_t> batch_size;
  std::vector<ssize_t> vocabulary_size;
  std::vector<ssize_t> nonzeros_a;
  std::vector<ssize_t> nonzeros_b;
  std::vector<ssize_t> num_epochs;
  std::vector<std::string> multiplication_types;
  ssize_t max_runs;
  // bool measure_communication;

  sgd_config() : mpc_config() {
    namespace po = boost::program_options;
    add_options()(
        "num_documents,l", po::value(&num_documents)->composing(),
        "Number of rows in the server's matrix; can be passed multiple times")(
        "batch_size", po::value(&batch_size)->composing(),
        "Number of documents to process at once; can be passed multiple times")(
        "vocabulary_size,m", po::value(&vocabulary_size)->composing(),
        "Number of columns in A and number of rows in B; can be passed "
        "multiple times")(
        "nonzeros_a,a", po::value(&nonzeros_a)->composing(),
        "Number of non-zero columns in each batch of the first party's matrix "
        "A; can be passed multiple times")(
        "nonzeros_b,b", po::value(&nonzeros_b)->composing(),
        "Number of non-zero columns in each batch of the second party's matrix "
        "B; can be passed multiple times")(
        "num_epochs,e", po::value(&num_epochs)->composing(),
        "Number of epochs to train for. Defaults to 1. Can be passed multiple "
        "times")(
        "multiplication_type", po::value(&multiplication_types)->composing(),
        "Multiplication type: dense | sparse; can be passed multiple times")(
        "max_runs", po::value(&max_runs)->default_value(-1),
        "Maximum number of runs. Default is unlimited");
    // TODO:
    // ("measure_communication",
    // po::bool_switch(&measure_communication)->default_value(false), "Measure
    // communication");
  }
};

// generates random matrices and multiplies them using multiplication triples
int main(int argc, const char* argv[]) {
  using T = uint64_t;
  int precision = 10;
  // parse config
  sgd_config conf;
  try {
    conf.parse(argc, argv);
  } catch (boost::program_options::error& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  // connect to other party
  party p(conf);
  auto channel = p.connect_to(1 - p.get_id());

  basic_oblivious_map<int, T> proto(channel);
  using dense_matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  int seed = 123456;  // seed random number generator deterministically
  std::mt19937 prg(seed);
  std::uniform_int_distribution<T> dist(0, 1000);

  // generate test data
  size_t num_experiments =
      std::max({conf.num_documents.size(), conf.batch_size.size(),
                conf.vocabulary_size.size(), conf.nonzeros_a.size(),
                conf.nonzeros_b.size(), conf.num_epochs.size(),
                conf.multiplication_types.size()});
  size_t n = 1;
  for (size_t num_runs = 0; conf.max_runs < 0 || num_runs < conf.max_runs;
       num_runs++) {
    for (size_t experiment = 0; experiment < num_experiments; experiment++) {
      std::cout << "Run " << num_runs << "\n";
      mpc_utils::Benchmarker benchmarker;
      size_t batch_size = get_ceil(conf.batch_size, experiment);
      size_t l = get_ceil(conf.num_documents, experiment);
      size_t m = get_ceil(conf.vocabulary_size, experiment);
      size_t k_A = get_ceil(conf.nonzeros_a, experiment);
      size_t k_B = get_ceil(conf.nonzeros_b, experiment);
      size_t num_epochs = get_ceil(conf.num_epochs, experiment);
      const std::string& mult_type =
          get_ceil(conf.multiplication_types, experiment);
      std::cout << "l = " << l << "\nm = " << m << "\nn = " << n
                << "\nk_A = " << k_A << "\nk_B = " << k_B
                << "\nbatch_size = " << batch_size
                << "\nmultiplication_type = " << mult_type << "\n";

      // Each party holds half of the data.
      Eigen::SparseMatrix<T, Eigen::RowMajor> A(l / 2, m);
      Eigen::SparseMatrix<T, Eigen::RowMajor> B(l / 2, m);
      Eigen::Matrix<T, Eigen::Dynamic, 1> A_labels(A.rows());
      Eigen::Matrix<T, Eigen::Dynamic, 1> B_labels(B.rows());
      Eigen::Matrix<T, Eigen::Dynamic, 1> model(m);
      std::cout << "Generating random data\n";

      std::vector<Eigen::Triplet<T>> triplets_A;
      auto indices_A = reservoir_sampling(prg, k_A, m);
      for (size_t i = 0; i < A.rows(); i++) {
        for (size_t j = 0; j < indices_A.size(); j++) {
          Eigen::Triplet<T> triplet(
              i, indices_A[j], dist(prg) * (static_cast<T>(1) << precision));
          triplets_A.push_back(triplet);
        }
        A_labels[i] = 0;
      }
      A.setFromTriplets(triplets_A.begin(), triplets_A.end());
      std::vector<Eigen::Triplet<T>> triplets_B;
      auto indices_B = reservoir_sampling(prg, k_B, m);
      for (size_t i = 0; i < B.rows(); i++) {
        for (size_t j = 0; j < indices_B.size(); j++) {
          Eigen::Triplet<T> triplet(
              i, indices_B[j], dist(prg) * (static_cast<T>(1) << precision));
          triplets_B.push_back(triplet);
        }
        B_labels[i] = 1;
      }
      B.setFromTriplets(triplets_B.begin(), triplets_B.end());

      for (int epoch = 0; epoch < num_epochs; epoch++) {
        int active_party = 0;  // Alternates between batches.
        const Eigen::SparseMatrix<T, Eigen::RowMajor>* input[2] = {&A, &B};
        const Eigen::Matrix<T, Eigen::Dynamic, 1>* labels[2] = {&A_labels,
                                                                &B_labels};
        const size_t sparsity[2] = {k_A, k_B};
        int row_index[2] = {0, 0};
        bool done[2] = {false, false};
        while (!done[0] && !done[1]) {
          // Compute batch size (last batch might differ).
          int this_batch_size = batch_size;
          if (row_index[active_party] + this_batch_size >
              input[active_party]->rows()) {
            this_batch_size =
                input[active_party]->rows() - row_index[active_party];
          }

          // Forward pass.
          Eigen::Matrix<T, Eigen::Dynamic, 1> activations(this_batch_size);
          try {
            if (mult_type == "dense") {
              fake_triple_provider<T> triples(this_batch_size, m, n,
                                              active_party == p.get_id());
              channel.sync();
              // TODO: aggregate fake triple computation times
              benchmarker.BenchmarkFunction("Fake Triple Generation",
                                            [&] { triples.precompute(1); });

              channel.sync();
              benchmarker.BenchmarkFunction("Forward Pass", [&] {
                activations = matrix_multiplication(
                    input[active_party]->middleRows(row_index[active_party],
                                                    this_batch_size),
                    model, channel, active_party == p.get_id(), triples,
                    this_batch_size);
              });
            } else if (mult_type == "sparse") {
              fake_triple_provider<T, true> triples(this_batch_size,
                                                    sparsity[active_party], n,
                                                    active_party == p.get_id());
              channel.sync();
              benchmarker.BenchmarkFunction("Fake Triple Generation",
                                            [&] { triples.precompute(1); });

              channel.sync();
              benchmarker.BenchmarkFunction("Forward Pass", [&] {
                activations = matrix_multiplication_cols_dense(
                    input[active_party]->middleRows(row_index[active_party],
                                                    this_batch_size),
                    model, proto, channel, active_party == p.get_id(), triples,
                    this_batch_size, sparsity[active_party], &benchmarker);
              });
            } else {
              BOOST_THROW_EXCEPTION(
                  std::runtime_error("Unknown multiplication_type"));
            }
          } catch (boost::exception& ex) {
            std::cerr << boost::diagnostic_information(ex);
            return 1;
          }
          if (p.get_id() == active_party) {
            activations += input[active_party]->middleRows(
                               row_index[active_party], this_batch_size) *
                           model;
          }

          // Rescale.
          activations /= (static_cast<T>(1) << precision);

          // Sigmoid.
          ProtocolDesc pd;
          if (channel.connect_to_oblivc(pd) == -1) {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("run_server: connection failed"));
          }
          setCurrentParty(&pd, 1 + p.get_id());
          std::vector<uint8_t> serialized_activations(sizeof(T) *
                                                      this_batch_size);
          serialize_le(serialized_activations.begin(), activations.data(),
                       this_batch_size);
          sigmoid_oblivc_args args = {
              .num_elements = this_batch_size,
              .element_size = sizeof(T),
              .precision = precision,
              .input = serialized_activations.data(),
              .output = serialized_activations.data(),
          };
          benchmarker.BenchmarkFunction(
              "Sigmoid", [&] { execYaoProtocol(&pd, sigmoid_oblivc, &args); });
          deserialize_le(activations.data(), serialized_activations.begin(),
                         this_batch_size);
          cleanupProtocol(&pd);

          // Compute difference to labels.
          if (p.get_id() == active_party) {
            activations -= labels[active_party]->middleRows(
                row_index[active_party], this_batch_size);
          }

          // Backward pass.
          Eigen::Matrix<T, Eigen::Dynamic, 1> gradient(m);
          try {
            if (mult_type == "dense") {
              fake_triple_provider<T> triples(m, this_batch_size, n,
                                              active_party == p.get_id());
              channel.sync();
              // TODO: aggregate fake triple computation times
              benchmarker.BenchmarkFunction("Fake Triple Generation",
                                            [&] { triples.precompute(1); });

              channel.sync();
              benchmarker.BenchmarkFunction("Backward Pass", [&] {
                gradient = matrix_multiplication(
                    input[active_party]
                        ->middleRows(row_index[active_party], this_batch_size)
                        .transpose(),
                    activations, channel, active_party == p.get_id(), triples,
                    -1);
              });
            } else if (mult_type == "sparse") {
              fake_triple_provider<T> triples(sparsity[active_party],
                                              this_batch_size, n,
                                              active_party == p.get_id());
              channel.sync();
              benchmarker.BenchmarkFunction("Fake Triple Generation",
                                            [&] { triples.precompute(1); });

              channel.sync();
              benchmarker.BenchmarkFunction("Backward Pass", [&] {
                gradient = matrix_multiplication_rows_dense(
                    input[active_party]
                        ->middleRows(row_index[active_party], this_batch_size)
                        .transpose(),
                    activations, channel, active_party == p.get_id(), triples,
                    -1, sparsity[active_party], &benchmarker);
              });
            } else {
              BOOST_THROW_EXCEPTION(
                  std::runtime_error("Unknown multiplication_type"));
            }
          } catch (boost::exception& ex) {
            std::cerr << boost::diagnostic_information(ex);
            return 1;
          }
          if (p.get_id() == active_party) {
            gradient +=
                input[active_party]
                    ->middleRows(row_index[active_party], this_batch_size)
                    .transpose() *
                activations;
          }

          // Rescale and divide by batch size.
          gradient /= this_batch_size;
          gradient /= (static_cast<T>(1) << precision);

          // Update model. TODO: learning rate.
          model += gradient;

          row_index[active_party] += this_batch_size;
          if (row_index[active_party] == input[active_party]->rows()) {
            done[active_party] = true;
          }
          if (!done[1 - active_party]) {
            active_party = 1 - active_party;
          }
        }
      }

      for (const auto& pair : benchmarker.GetAll()) {
        std::cout << pair.first << ": " << pair.second << "\n";
      }
    }
  }
}
