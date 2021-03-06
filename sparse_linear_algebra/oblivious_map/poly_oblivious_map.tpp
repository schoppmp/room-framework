#include <algorithm>
#include "absl/strings/str_cat.h"
#include "boost/range.hpp"
#include "boost/range/algorithm.hpp"
#include "fastpoly/recursive.h"
#include "mpc_utils/boost_serialization/ntl.hpp"
#include "mpc_utils/comm_channel_oblivc_adapter.hpp"
#include "sparse_linear_algebra/util/serialize_le.hpp"
#include "sparse_linear_algebra/util/time.h"
extern "C" {
#include "obliv.h"
#include "poly_oblivious_map.h"
}

template <typename K, typename V>
void poly_oblivious_map<K, V>::run_server(
    const poly_oblivious_map<K, V>::pair_range input,
    const poly_oblivious_map<K, V>::value_range defaults, bool shared_output,
    mpc_utils::Benchmarker* benchmarker) {
  try {
    mpc_utils::Benchmarker::time_point start;
    if (benchmarker != nullptr) {
      start = benchmarker->StartTimer();
    }

    NTL::ZZ_pPush push(modulus);
    nonce++;
    NTL::ZZ_pX poly_server;
    size_t input_length = boost::size(input);
    size_t default_length = boost::size(defaults);

    // convert server inputs
    NTL::Vec<NTL::ZZ_p> elements_server;
    NTL::Vec<NTL::ZZ_p> values_server;
    elements_server.SetMaxLength(input_length);
    values_server.SetMaxLength(input_length);
    for (auto pair : input) {
      elements_server.append(NTL::conv<NTL::ZZ_p>(pair.first));
      values_server.append(NTL::conv<NTL::ZZ_p>(pair.second));
    }

    // setup encryption
    if (key.size() == 0) {
      key.resize(block_size);
      gcry_randomize(key.data(), block_size, GCRY_STRONG_RANDOM);
    }
    gcry_cipher_hd_t handle;
    gcry_cipher_open(&handle, cipher, GCRY_CIPHER_MODE_CTR, 0);
    gcry_cipher_setkey(handle, key.data(), block_size);

    // encrypt server values
    for (size_t i = 0; i < input_length; i++) {
      NTL::ZZ val;
      NTL::conv(val, values_server[i]);
      val <<= statistical_security;
      // use AES counter mode with the element as the counter
      unsigned char buf[block_size] = {0};
      unsigned char ctr[block_size] = {0};
      NTL::BytesFromZZ(
          ctr,
          (NTL::conv<NTL::ZZ>(elements_server[i]) << 8 * (sizeof(nonce))) +
              nonce,
          block_size);
      gcry_cipher_setctr(handle, ctr, block_size);
      if (NTL::NumBytes(val) > block_size) {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Server value does not fit in plaintext space"));
      }
      NTL::BytesFromZZ(buf, val, block_size);
      gcry_cipher_encrypt(handle, buf, block_size, nullptr, 0);
      NTL::conv(values_server[i], NTL::ZZFromBytes(buf, block_size));
    }
    gcry_cipher_close(handle);

    // interpolate polynomial over the values
    // interpolate_recursive(poly_server, elements_server,values_server);
    poly_interpolate_zp_recursive(values_server.length() - 1,
                                  elements_server.data(), values_server.data(),
                                  poly_server);
    chan.send(poly_server);
    chan.flush();

    // set up inputs for obliv-c
    std::vector<uint8_t> defaults_bytes(default_length * sizeof(V), 0);
    serialize_le(defaults_bytes.begin(), boost::begin(defaults),
                 default_length);
    pir_poly_oblivc_args args = {.statistical_security = statistical_security,
                                 .value_type_size = sizeof(V),
                                 .input_size = key.size(),
                                 .input = key.data(),
                                 .defaults = defaults_bytes.data(),
                                 .result = nullptr,
                                 .shared_output = shared_output};

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("local_time", start);
      start = benchmarker->StartTimer();
    }

    // run yao's protocol using Obliv-C
    auto status =
        mpc_utils::CommChannelOblivCAdapter::Connect(chan, /*sleep_time=*/10);
    if (!status.ok()) {
      std::string error = absl::StrCat("run_server: connection failed: ",
                                       status.status().message());
      BOOST_THROW_EXCEPTION(std::runtime_error(error));
    }
    ProtocolDesc pd = status.ValueOrDie();
    setCurrentParty(&pd, 1);
    execYaoProtocol(&pd, pir_poly_oblivc, &args);

    if (benchmarker != nullptr && chan.is_measured()) {
      benchmarker->AddAmount("Bytes Sent (Obliv-C)", tcp2PBytesSent(&pd));
    }

    cleanupProtocol(&pd);

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("mpc_time", start);
      start = benchmarker->StartTimer();
    }
  } catch (NTL::ErrorObject& ex) {
    BOOST_THROW_EXCEPTION(ex);
  }
}

template <typename K, typename V>
void poly_oblivious_map<K, V>::run_client(
    const poly_oblivious_map<K, V>::key_range input,
    const poly_oblivious_map<K, V>::value_range output, bool shared_output,
    mpc_utils::Benchmarker* benchmarker) {
  try {
    mpc_utils::Benchmarker::time_point start;
    if (benchmarker != nullptr) {
      start = benchmarker->StartTimer();
    }

    NTL::ZZ_pPush push(modulus);
    nonce++;
    size_t length = boost::size(input);

    // receive polynomial from server and send number of inputs
    NTL::ZZ_pX poly_server;
    chan.recv(poly_server);

    // convert client inputs to NTL vectors
    NTL::Vec<NTL::ZZ_p> elements_client;
    NTL::Vec<NTL::ZZ_p> values_client;
    elements_client.SetLength(length);
    values_client.SetLength(length);
    boost::copy(input, elements_client.begin());
    // evaluate polynomial using fastpoly
    poly_evaluate_zp_recursive(elements_client.length() - 1, poly_server,
                               elements_client.data(), values_client.data());

    std::vector<uint8_t> result(values_client.length() * sizeof(V));
    // set up inputs for obliv-c
    std::vector<uint8_t> ciphertexts_client(values_client.length() * 2 *
                                            block_size);
    pir_poly_oblivc_args args = {.statistical_security = statistical_security,
                                 .value_type_size = sizeof(V),
                                 .input_size = ciphertexts_client.size(),
                                 .input = ciphertexts_client.data(),
                                 .defaults = nullptr,
                                 .result = result.data(),
                                 .shared_output = shared_output};
    // serialize ciphertexts and elements (used as ctr in decryption)
    for (size_t i = 0; i < values_client.length(); i++) {
      NTL::BytesFromZZ(ciphertexts_client.data() + i * 2 * block_size,
                       NTL::conv<NTL::ZZ>(values_client[i]), block_size);
      NTL::BytesFromZZ(
          ciphertexts_client.data() + (i * 2 + 1) * block_size,
          (NTL::conv<NTL::ZZ>(elements_client[i]) << 8 * (sizeof(nonce))) +
              nonce,
          block_size);
    }
    chan.flush();

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("local_time", start);
      start = benchmarker->StartTimer();
    }

    // run yao's protocol using Obliv-C
    auto status =
        mpc_utils::CommChannelOblivCAdapter::Connect(chan, /*sleep_time=*/10);
    if (!status.ok()) {
      std::string error = absl::StrCat("run_client: connection failed: ",
                                       status.status().message());
      BOOST_THROW_EXCEPTION(std::runtime_error(error));
    }
    ProtocolDesc pd = status.ValueOrDie();
    setCurrentParty(&pd, 2);
    execYaoProtocol(&pd, pir_poly_oblivc, &args);

    if (benchmarker != nullptr && chan.is_measured()) {
      benchmarker->AddAmount("Bytes Sent (Obliv-C)", tcp2PBytesSent(&pd));
    }

    cleanupProtocol(&pd);

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("mpc_time", start);
      start = benchmarker->StartTimer();
    }

    deserialize_le(boost::begin(output), result.data(), length);

    if (benchmarker != nullptr) {
      benchmarker->AddSecondsSinceStart("local_time", start);
    }
  } catch (NTL::ErrorObject& ex) {
    BOOST_THROW_EXCEPTION(ex);
  }
}
