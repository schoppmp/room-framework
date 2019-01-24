extern "C" {
#include "knn.h"
}
#include "knn_protocol.hpp"
#include "mpc_utils/mpc_config.hpp"
#include "sparse_linear_algebra/matrix_multiplication/dense.hpp"
#include "sparse_linear_algebra/matrix_multiplication/sparse/cols-dense.hpp"
#include "sparse_linear_algebra/matrix_multiplication/sparse/cols-rows.hpp"
#include "sparse_linear_algebra/matrix_multiplication/sparse/rows-dense.hpp"
#include "sparse_linear_algebra/oblivious_map/basic_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/fss_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/poly_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/sorting_oblivious_map.hpp"
#include "sparse_linear_algebra/util/get_ceil.hpp"
#include "sparse_linear_algebra/util/randomize_matrix.hpp"
#include "sparse_linear_algebra/util/reservoir_sampling.hpp"
#include "sparse_linear_algebra/util/time.h"

namespace sparse_linear_algebra::applications::knn {
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
KNNProtocol::KNNProtocol(comm_channel* channel, int party_id,
                         int16_t statistical_security, int precision,
                         MulType mt, PirType pt, size_t chunk_size, size_t k,
                         size_t num_documents_server, size_t num_words,
                         size_t num_documents_client,
                         size_t num_nonzeros_server, size_t num_nonzeros_client,
                         Eigen::SparseMatrix<T, Eigen::RowMajor> server_matrix,
                         Eigen::SparseMatrix<T, Eigen::ColMajor> client_matrix)
    : channel(channel),
      party_id(party_id),
      pir_protocols({
          {basic,
           std::make_shared<basic_oblivious_map<int, int>>(*channel, true)},
          {poly, std::make_shared<poly_oblivious_map<int, int>>(
                     *channel, statistical_security, true)},
          {scs, std::make_shared<sorting_oblivious_map<int, int>>(*channel, true)},
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
  std::cout << "num_documents_server = " << num_documents_server
            << "\nm = " << num_words
            << "\nnum_documents_client = " << num_documents_client
            << "\nnum_nonzeros_server = " << num_nonzeros_server
            << "\nnum_nonzeros_client = " << num_nonzeros_client
            << "\nchunk_size = " << chunk_size << "\npir_type = " << pt
            << "\nmultiplication_type = " << mt << "\n";
}

std::vector<int> KNNProtocol::run() {
  computeSimilarities();
  return topK();
}

void KNNProtocol::computeSimilarities() {
  for (int row = 0; row < num_documents_server; row += chunk_size) {
    // The last chunk might be smaller.
    int this_chunk_size = chunk_size;
    if (row + this_chunk_size > num_documents_server) {
      this_chunk_size = num_documents_server - row;
    }
    // Avoid out-of-memory errors by using a different chunk size for dense
    // matrix multiplication.
    int dense_chunk_size = this_chunk_size;
    if (dense_chunk_size > 4096) {
      dense_chunk_size = 2048;
    }
    switch (this->mt) {
      case dense: {
        fake_triple_provider<T> triples(dense_chunk_size, num_words,
                                        num_documents_client, party_id);
        channel->sync();
        // TODO: aggregate fake triple computation times
        benchmark(
            [&] {
              triples.precompute((this_chunk_size + dense_chunk_size - 1) /
                                 dense_chunk_size);
            },
            "Fake Triple Generation");

        channel->sync();
        benchmark(
            [&] {
              result_matrix.middleRows(row, this_chunk_size) =
                  matrix_multiplication(
                      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>(
                          server_matrix)
                          .middleRows(row, this_chunk_size),
                      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>(
                          client_matrix),
                      *channel, party_id, triples, dense_chunk_size);
            },
            "Total");
        break;
      }
      case sparse: {
        fake_triple_provider<T> triples(
            dense_chunk_size, num_nonzeros_server + num_nonzeros_client,
            num_documents_client, party_id);
        channel->sync();
        benchmark(
            [&] {
              triples.precompute((this_chunk_size + dense_chunk_size - 1) /
                                 dense_chunk_size);
            },
            "Fake Triple Generation");

        channel->sync();
        benchmark(
            [&] {
              result_matrix.middleRows(row, this_chunk_size) =
                  matrix_multiplication_cols_rows(
                      server_matrix.middleRows(row, this_chunk_size),
                      client_matrix, *pir_protocols[pt], *channel, party_id,
                      triples, dense_chunk_size, num_nonzeros_server,
                      num_nonzeros_client, true);
            },
            "Total");
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

std::vector<int> KNNProtocol::topK() {
  // compute norms
  std::vector<T> norms_A(num_documents_server);
  for (int i = 0; i < num_documents_server; i++) {
    norms_A[i] =
        (server_matrix.row(i) / (static_cast<T>(1) << precision)).norm();
  }
  T norm_B = (client_matrix / (static_cast<T>(1) << precision)).norm();

  // run top-k selection
  ProtocolDesc pd;
  if (channel->connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1 + party_id);
  std::vector<uint8_t> serialized_inputs(num_documents_server * sizeof(T));
  std::vector<uint8_t> serialized_norms;
  std::vector<uint8_t> serialized_outputs(k * sizeof(int));
  std::vector<T> inputs_vec(num_words);
  std::vector<int> outputs(k);
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
  execYaoProtocol(&pd, knn_oblivc, &args);
  // TODO return this instead of printing it?
  std::cout << "Bytes sent: " << tcp2PBytesSent(&pd) << "\n";
  cleanupProtocol(&pd);
  return outputs;
}

}  // namespace sparse_linear_algebra::applications::knn