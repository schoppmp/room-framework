#include "sparse_linear_algebra/applications/knn/knn_protocol.hpp"
#include "absl/strings/str_cat.h"
#include "mpc_utils/comm_channel_oblivc_adapter.hpp"
#include "mpc_utils/mpc_config.hpp"
#include "sparse_linear_algebra/matrix_multiplication/cols-dense.hpp"
#include "sparse_linear_algebra/matrix_multiplication/cols-rows.hpp"
#include "sparse_linear_algebra/matrix_multiplication/dense.hpp"
#include "sparse_linear_algebra/matrix_multiplication/offline/fake_triple_provider.hpp"
#include "sparse_linear_algebra/matrix_multiplication/rows-dense.hpp"
#include "sparse_linear_algebra/oblivious_map/basic_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/poly_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/sorting_oblivious_map.hpp"
#include "sparse_linear_algebra/util/get_ceil.hpp"
#include "sparse_linear_algebra/util/randomize_matrix.hpp"
#include "sparse_linear_algebra/util/reservoir_sampling.hpp"
#include "sparse_linear_algebra/util/time.h"

extern "C" {
#include "top_k.h"
}

namespace sparse_linear_algebra {
namespace applications {
namespace knn {

// Does a complete run of the knn protocol.
// Example:
//    // optional: generate random documents for client and server respectively
//    as normal distributed matrices.
//    // IMPORTANT: At the moment, only a single client matrix is supported,
//    which is reflected in this example.
//
//    Eigen::SparseMatrix<T, Eigen::RowMajor> server_matrix(
//        num_documents_server, num_words_in_vocabulary);
//    Eigen::SparseMatrix<T, Eigen::ColMajor> client_matrix(
//        num_words_in_vocabulary, 1);
//    generateRandomMatrices(123456, precision, num_documents_server, m, n,
//                           num_nonzeros_server, num_nonzeros_client,
//                           &server_matrix, &client_matrix);
//    // instantiate the protocol
//    KNNProtocol protocol(channel, party_id, statistical_security, precision,
//                         mt, pt, chunk_size, num_top_k, num_documents_server,
//                         num_words_in_vocabulary, 1, num_nonzeros_server,
//                         num_nonzeros_client, server_matrix, client_matrix);
//    // Run the protocol. Result are the indices for the server's top k
//    documents
//    // that are most similiar to the client's document.
//    auto result = protocol.run();
template <typename T>
KNNProtocol<T>::KNNProtocol(
    comm_channel* channel, int party_id, int16_t statistical_security,
    int precision, MulType mt, PirType pt, int chunk_size, int k,
    int64_t num_documents_server, int64_t num_words,
    int64_t num_documents_client, int num_nonzeros_server,
    int num_nonzeros_client,
    Eigen::SparseMatrix<T, Eigen::RowMajor> server_matrix,
    Eigen::SparseMatrix<T, Eigen::ColMajor> client_matrix)
    : channel_(channel),
      party_id_(party_id),
      pir_protocols_({
          {basic, std::make_shared<basic_oblivious_map<int, int>>(*channel)},
          {poly, std::make_shared<poly_oblivious_map<int, int>>(
                     *channel, statistical_security, true)},
          {scs, std::make_shared<sorting_oblivious_map<int, int>>(*channel)},
      }),
      precision_(precision),
      mul_type_(mt),
      pir_type_(pt),
      chunk_size_(chunk_size),
      k_(k),
      num_documents_server_(num_documents_server),
      num_words_(num_words),
      num_documents_client_(num_documents_client),
      num_nonzeros_server_(num_nonzeros_server),
      num_nonzeros_client_(num_nonzeros_client),
      server_matrix_(server_matrix),
      client_matrix_(client_matrix),
      result_matrix_(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>(
          num_documents_server, num_documents_client)) {
  // TODO: assert size of input matrices match inputted sizes
  assert(num_documents_client == 1);

  int64_t num_nonzeros_batch_avg = 0;

  // Compute actual number of nonzeros per batch if none was entered by the
  // user.
  if (num_nonzeros_server < 0) {
    if (party_id == 0) {
      for (int row = 0; row < num_documents_server; row += chunk_size) {
        // The last chunk might be smaller.
        int this_chunk_size = chunk_size;
        if (row + this_chunk_size > num_documents_server) {
          this_chunk_size = num_documents_server - row;
        }
        Eigen::SparseMatrix<T, Eigen::RowMajor> this_chunk =
            server_matrix_.middleRows(row, this_chunk_size);
        this_chunk.makeCompressed();
        std::unordered_set<size_t> indices(
            this_chunk.innerIndexPtr(),
            this_chunk.innerIndexPtr() + this_chunk.nonZeros());
        int num_nonzeros_batch = static_cast<int>(indices.size());
        num_nonzeros_server_ =
            std::max(num_nonzeros_server_, num_nonzeros_batch);
        num_nonzeros_batch_avg += num_nonzeros_batch;
      }
      channel->send(num_nonzeros_server_);
    } else {
      channel->recv(num_nonzeros_server_);
    }
  }
  std::cout << "num_documents_server_batch_avg = "
            << ((double)num_nonzeros_batch_avg) /
                   ((double)num_documents_server / chunk_size)
            << "\n";

  std::cout << "num_documents_server_batch_max = " << num_nonzeros_server_
            << "\n";
}

template <typename T>
std::vector<int> KNNProtocol<T>::run(mpc_utils::Benchmarker* benchmarker) {
  computeSimilarities(benchmarker);
  return topK(benchmarker);
}

template <typename T>
void KNNProtocol<T>::computeSimilarities(mpc_utils::Benchmarker* benchmarker) {
  using sparse_linear_algebra::matrix_multiplication::offline::
      FakeTripleProvider;

  for (int row = 0; row < num_documents_server_; row += chunk_size_) {
    // The last chunk might be smaller.
    int this_chunk_size = chunk_size_;
    if (row + this_chunk_size > num_documents_server_) {
      this_chunk_size = num_documents_server_ - row;
    }
    int dense_chunk_size = this_chunk_size;
    if (dense_chunk_size > 4096) {
      dense_chunk_size = 2048;
    }
    switch (this->mul_type_) {
      case dense: {
        FakeTripleProvider<T> triples(dense_chunk_size, num_words_,
                                      num_documents_client_, party_id_);
        channel_->sync();
        mpc_utils::Benchmarker::MaybeBenchmarkFunction(
            benchmarker, "Fake Triple Generation", [&] {
              triples.Precompute((this_chunk_size + dense_chunk_size - 1) /
                                 dense_chunk_size);
            });

        channel_->sync();
        mpc_utils::Benchmarker::MaybeBenchmarkFunction(
            benchmarker, "Matrix Multiplication", [&] {
              result_matrix_.middleRows(row, this_chunk_size) =
                  matrix_multiplication_dense(
                      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>(
                          server_matrix_.middleRows(row, this_chunk_size)),
                      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>(
                          client_matrix_),
                      *channel_, party_id_, triples, dense_chunk_size);
            });
        break;
      }
      case sparse: {
        FakeTripleProvider<T> triples(
            dense_chunk_size, num_nonzeros_server_ + num_nonzeros_client_,
            num_documents_client_, party_id_);
        channel_->sync();
        mpc_utils::Benchmarker::MaybeBenchmarkFunction(
            benchmarker, "Fake Triple Generation", [&] {
              triples.Precompute((this_chunk_size + dense_chunk_size - 1) /
                                 dense_chunk_size);
            });

        channel_->sync();
        mpc_utils::Benchmarker::MaybeBenchmarkFunction(
            benchmarker, "Matrix Multiplication", [&] {
              result_matrix_.middleRows(row, this_chunk_size) =
                  matrix_multiplication_cols_rows(
                      server_matrix_.middleRows(row, this_chunk_size),
                      client_matrix_, *pir_protocols_[pir_type_], *channel_,
                      party_id_, triples, dense_chunk_size,
                      num_nonzeros_server_, num_nonzeros_client_, benchmarker);
            });
        break;
      }
      default: {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Unreachable. Please report this to the authors."));
        break;
      }
    }
  }
}

template <typename T>
std::vector<int> KNNProtocol<T>::topK(mpc_utils::Benchmarker* benchmarker) {
  // compute norms
  std::vector<T> norms_A(num_documents_server_);
  for (int i = 0; i < num_documents_server_; i++) {
    norms_A[i] =
        (server_matrix_.row(i) / (static_cast<T>(1) << precision_)).norm();
  }
  T norm_B = (client_matrix_ / (static_cast<T>(1) << precision_)).norm();
  std::vector<int> outputs(k_);

  mpc_utils::Benchmarker::MaybeBenchmarkFunction(benchmarker, "Top-K", [&] {
    // run top-k selection
    auto status = mpc_utils::CommChannelOblivCAdapter::Connect(
        *channel_, /*sleep_time=*/10);
    if (!status.ok()) {
      std::string error =
          absl::StrCat("topK: connection failed: ", status.status().message());
      BOOST_THROW_EXCEPTION(std::runtime_error(error));
    }
    ProtocolDesc pd = status.ValueOrDie();
    setCurrentParty(&pd, 1 + party_id_);
    std::vector<uint8_t> serialized_inputs(num_documents_server_ * sizeof(T));
    std::vector<uint8_t> serialized_norms;
    std::vector<uint8_t> serialized_outputs(k_ * sizeof(int));
    std::vector<T> inputs_vec(num_documents_server_);
    for (int i = 0; i < num_documents_server_; i++) {
      inputs_vec[i] = result_matrix_(i, 0);
    }
    serialize_le(serialized_inputs.begin(), inputs_vec.begin(),
                 num_documents_server_);
    if (party_id_ == 0) {
      serialized_norms.resize(num_documents_server_ * sizeof(T));
      serialize_le(serialized_norms.begin(), norms_A.begin(),
                   num_documents_server_);
    } else {
      serialized_norms.resize(sizeof(T));
      serialize_le(serialized_norms.begin(), &norm_B, 1);
    }
    knn_oblivc_args args = {static_cast<int>(num_documents_server_),
                            static_cast<int>(k_),
                            sizeof(T),
                            serialized_inputs.data(),
                            serialized_norms.data(),
                            outputs.data()};
    channel_->sync();
    execYaoProtocol(&pd, top_k_oblivc, &args);

    if (benchmarker && channel_->is_measured()) {
      benchmarker->AddAmount("Bytes Sent (Obliv-C)", tcp2PBytesSent(&pd));
    }

    cleanupProtocol(&pd);
  });
  return outputs;
}

// Template instantiations
template class KNNProtocol<uint64_t>;

}  // namespace knn
}  // namespace applications
}  // namespace sparse_linear_algebra