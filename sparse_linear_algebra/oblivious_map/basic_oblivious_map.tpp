#include <algorithm>
#include "boost/range.hpp"
#include "boost/range/algorithm.hpp"
#include "sparse_linear_algebra/util/serialize_le.hpp"
#include "sparse_linear_algebra/util/time.h"
extern "C" {
#include "basic_oblivious_map.h"
#include "obliv.h"
}

template <typename K, typename V>
void basic_oblivious_map<K, V>::run_server(
    const basic_oblivious_map<K, V>::pair_range input,
    const basic_oblivious_map<K, V>::value_range defaults, bool shared_output,
    mpc_utils::Benchmarker* benchmarker) {
  mpc_utils::Benchmarker::time_point start;
  if (benchmarker != nullptr) {
    start = benchmarker->StartTimer();
  }
  size_t input_length = boost::size(input);
  size_t default_length = boost::size(defaults);
  // write values into a dense vector
  std::vector<uint8_t> input_bytes, defaults_bytes(default_length * sizeof(V));
  serialize_le(defaults_bytes.data(), std::begin(defaults), default_length);
  input_bytes.reserve(256);
  size_t max_key = 0;
  auto it = boost::begin(input);
  for (size_t i = 0; i < input_length; i++, it++) {
    K current_key = it->first;
    while (input_bytes.capacity() <= current_key * (sizeof(V) + 1)) {
      input_bytes.reserve(2 * input_bytes.capacity());
    }
    if (current_key > max_key) {
      max_key = current_key;
      input_bytes.resize((current_key + 1) * (sizeof(V) + 1), 0);
    }
    serialize_le(&input_bytes[current_key * (sizeof(V) + 1)], &(it->second), 1);
    // one extra byte per element to distinguish missing values from zeros
    input_bytes[current_key * (sizeof(V) + 1) + sizeof(V)] = 1;
  }

  // setup encryption
  if (key.size() == 0) {
    key.resize(block_size);
    gcry_randomize(key.data(), block_size, GCRY_STRONG_RANDOM);
  }
  gcry_cipher_hd_t handle;
  gcry_cipher_open(&handle, cipher, GCRY_CIPHER_MODE_CTR, 0);
  gcry_cipher_setkey(handle, key.data(), block_size);

  // encrypt
  for (size_t i = 0; i < max_key + 1; i++) {
    uint8_t buf[block_size] = {0};
    uint8_t ctr[block_size] = {0};
    serialize_le(&ctr[0], &i, 1);
    gcry_cipher_setctr(handle, ctr, block_size);
    gcry_cipher_encrypt(handle, buf, block_size, nullptr, 0);
    for (size_t j = 0; j < sizeof(V) + 1; j++) {
      input_bytes[i * (sizeof(V) + 1) + j] ^= buf[j];
    }
  }
  gcry_cipher_close(handle);

  // send encrypted vector to client for selection
  chan.send(input_bytes);

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("local_time", start);
    start = benchmarker->StartTimer();
  }

  // setup obliv-c inputs
  pir_basic_oblivc_args args = {.index_size = sizeof(K),
                                .element_size = sizeof(V),
                                .num_ciphertexts = default_length,
                                .indexes_client = nullptr,
                                .ciphertexts_client = nullptr,
                                .defaults_server = defaults_bytes.data(),
                                .key_server = key.data(),
                                .result = nullptr,
                                .shared_output = shared_output};
  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  if (chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1);
  execYaoProtocol(&pd, pir_basic_oblivc, &args);

  if (benchmarker != nullptr && chan.is_measured()) {
    benchmarker->AddAmount("Bytes Sent (Obliv-C)", tcp2PBytesSent(&pd));
  }

  cleanupProtocol(&pd);

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("mpc_time", start);
  }
}

template <typename K, typename V>
void basic_oblivious_map<K, V>::run_client(
    const basic_oblivious_map<K, V>::key_range input,
    const basic_oblivious_map<K, V>::value_range output, bool shared_output,
    mpc_utils::Benchmarker* benchmarker) {
  mpc_utils::Benchmarker::time_point start;
  if (benchmarker != nullptr) {
    start = benchmarker->StartTimer();
  }
  size_t length = boost::size(input);
  std::vector<uint8_t> all_ciphertexts;
  chan.recv(all_ciphertexts);
  size_t num_all_ciphertexts = all_ciphertexts.size() / (sizeof(V) + 1);
  std::vector<uint8_t> selected_ciphertexts(length * (sizeof(V) + 1), 0);
  std::vector<uint8_t> indexes_bytes(length * (sizeof(K) + 1), 0);
  auto it = boost::begin(input);
  for (size_t i = 0; i < length; i++, it++) {
    K cur_index = *it;
    serialize_le(&indexes_bytes[i * (sizeof(K) + 1)], &cur_index, 1);
    if (cur_index < num_all_ciphertexts) {
      std::copy_n(&all_ciphertexts[cur_index * (sizeof(V) + 1)], sizeof(V) + 1,
                  &selected_ciphertexts[i * (sizeof(V) + 1)]);
      indexes_bytes[i * (sizeof(K) + 1) + sizeof(K)] = 1;
    } else {  // index outside of range from the server -> unset valid flag
      indexes_bytes[i * (sizeof(K) + 1) + sizeof(K)] = 0;
    }
  }

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("local_time", start);
    start = benchmarker->StartTimer();
  }

  // setup obliv-c arguments
  std::vector<uint8_t> result_bytes(sizeof(V) * length);
  pir_basic_oblivc_args args = {
      .index_size = sizeof(K),
      .element_size = sizeof(V),
      .num_ciphertexts = length,
      .indexes_client = indexes_bytes.data(),
      .ciphertexts_client = selected_ciphertexts.data(),
      .defaults_server = nullptr,
      .key_server = nullptr,
      .result = result_bytes.data(),
      .shared_output = shared_output};
  // run yao's protocol using Obliv-C
  ProtocolDesc pd;
  if (chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_client: connection failed"));
  }
  setCurrentParty(&pd, 2);
  execYaoProtocol(&pd, pir_basic_oblivc, &args);

  if (benchmarker != nullptr && chan.is_measured()) {
    benchmarker->AddAmount("Bytes Sent (Obliv-C)", tcp2PBytesSent(&pd));
  }

  cleanupProtocol(&pd);

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("mpc_time", start);
    start = benchmarker->StartTimer();
  }

  deserialize_le(output.begin(), result_bytes.data(), length);

  if (benchmarker != nullptr) {
    benchmarker->AddSecondsSinceStart("local_time", start);
  }
}
