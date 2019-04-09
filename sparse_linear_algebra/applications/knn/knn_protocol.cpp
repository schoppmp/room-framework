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
    int precision, MulType mt, PirType pt, size_t chunk_size, size_t k,
    size_t num_documents_server, size_t num_words, size_t num_documents_client,
    size_t num_nonzeros_server, size_t num_nonzeros_client,
    Eigen::SparseMatrix<T, Eigen::RowMajor> server_matrix,
    Eigen::SparseMatrix<T, Eigen::ColMajor> client_matrix)
    : channel(channel),
      party_id(party_id),
      pir_protocols({
          {basic, std::make_shared<basic_oblivious_map<int, int>>(*channel)},
          {poly, std::make_shared<poly_oblivious_map<int, int>>(
                     *channel, statistical_security, true)},
          {scs, std::make_shared<sorting_oblivious_map<int, int>>(*channel)},
      }),
      precision(precision),
      mt(mt),
      pt(pt),
      chunk_size(chunk_size),
      k(k),
      num_documents_server(num_documents_server),
      num_words(num_words),
      num_documents_client(num_documents_client),
      num_nonzeros_server(num_nonzeros_server),
      num_nonzeros_client(num_nonzeros_client),
      server_matrix(server_matrix),
      client_matrix(client_matrix),
      result_matrix(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>(
          num_documents_server, num_documents_client)) {
  // TODO: assert size of input matrices match inputted sizes
  assert(num_documents_client == 1);
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

  for (int row = 0; row < num_documents_server; row += chunk_size) {
    // The last chunk might be smaller.
    int this_chunk_size = chunk_size;
    if (row + this_chunk_size > num_documents_server) {
      this_chunk_size = num_documents_server - row;
    }
    int dense_chunk_size = this_chunk_size;
    //    Avoid out-of-memory errors by using a different chunk size for dense
    //    matrix multiplication.
    //    if (dense_chunk_size > 4096) {
    //      dense_chunk_size = 2048;
    //    }
    switch (this->mt) {
      case dense: {
        FakeTripleProvider<T> triples(dense_chunk_size, num_words,
                                      num_documents_client, party_id);
        channel->sync();
        mpc_utils::Benchmarker::MaybeBenchmarkFunction(
            benchmarker, "Fake Triple Generation", [&] {
              triples.Precompute((this_chunk_size + dense_chunk_size - 1) /
                                 dense_chunk_size);
            });

        channel->sync();
        mpc_utils::Benchmarker::MaybeBenchmarkFunction(
            benchmarker, "Matrix Multiplication", [&] {
              result_matrix.middleRows(row, this_chunk_size) =
                  matrix_multiplication_dense(
                      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>(
                          server_matrix.middleRows(row, this_chunk_size)),
                      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>(
                          client_matrix),
                      *channel, party_id, triples, dense_chunk_size);
            });
        break;
      }
      case sparse: {
        FakeTripleProvider<T> triples(dense_chunk_size,
                                      num_nonzeros_server + num_nonzeros_client,
                                      num_documents_client, party_id);
        channel->sync();
        mpc_utils::Benchmarker::MaybeBenchmarkFunction(
            benchmarker, "Fake Triple Generation", [&] {
              triples.Precompute((this_chunk_size + dense_chunk_size - 1) /
                                 dense_chunk_size);
            });

        channel->sync();
        mpc_utils::Benchmarker::MaybeBenchmarkFunction(
            benchmarker, "Matrix Multiplication", [&] {
              result_matrix.middleRows(row, this_chunk_size) =
                  matrix_multiplication_cols_rows(
                      server_matrix.middleRows(row, this_chunk_size),
                      client_matrix, *pir_protocols[pt], *channel, party_id,
                      triples, dense_chunk_size, num_nonzeros_server,
                      num_nonzeros_client, benchmarker);
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
  std::vector<T> norms_A(num_documents_server);
  for (int i = 0; i < num_documents_server; i++) {
    norms_A[i] =
        (server_matrix.row(i) / (static_cast<T>(1) << precision)).norm();
  }
  T norm_B = (client_matrix / (static_cast<T>(1) << precision)).norm();
  std::vector<int> outputs(k);

  mpc_utils::Benchmarker::MaybeBenchmarkFunction(benchmarker, "Top-K", [&] {
    // run top-k selection
    auto status = mpc_utils::CommChannelOblivCAdapter::Connect(
        *channel, /*sleep_time=*/10);
    if (!status.ok()) {
      std::string error =
          absl::StrCat("topK: connection failed: ", status.status().message());
      BOOST_THROW_EXCEPTION(std::runtime_error(error));
    }
    ProtocolDesc pd = status.ValueOrDie();
    setCurrentParty(&pd, 1 + party_id);
    std::vector<uint8_t> serialized_inputs(num_documents_server * sizeof(T));
    std::vector<uint8_t> serialized_norms;
    std::vector<uint8_t> serialized_outputs(k * sizeof(int));
    std::vector<T> inputs_vec(num_words);
    for (int i = 0; i < num_documents_server; i++) {
      inputs_vec[i] = result_matrix(i, 0);
    }
    serialize_le(serialized_inputs.begin(), inputs_vec.begin(),
                 num_documents_server);
    if (party_id == 0) {
      serialized_norms.resize(num_documents_server * sizeof(T));
      serialize_le(serialized_norms.begin(), norms_A.begin(),
                   num_documents_server);
    } else {
      serialized_norms.resize(sizeof(T));
      serialize_le(serialized_norms.begin(), &norm_B, num_documents_client);
    }
    knn_oblivc_args args = {static_cast<int>(num_documents_server),
                            static_cast<int>(k),
                            sizeof(T),
                            serialized_inputs.data(),
                            serialized_norms.data(),
                            outputs.data()};
    execYaoProtocol(&pd, top_k_oblivc, &args);

    if (benchmarker && channel->is_measured()) {
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