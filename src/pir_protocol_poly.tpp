#include <algorithm>

#include "mpc-utils/boost_serialization.hpp"
#include "fastpoly/recursive.h"
extern "C" {
  #include <obliv.h>
  #include "pir_protocol_poly.h"
}

template<typename K, typename V>
void pir_protocol_poly<K, V>::run_server(const std::map<K,V>& server_in, std::vector<V>& server_out) {
  try {
    NTL::ZZ_pPush push(modulus);
    nonce++;
    NTL::ZZ_pX poly_server;

    // convert server inputs
    NTL::Vec<NTL::ZZ_p> elements_server;
    NTL::Vec<NTL::ZZ_p> values_server;
    elements_server.SetMaxLength(server_in.size());
    values_server.SetMaxLength(server_in.size());
    for(auto& pair : server_in) {
      elements_server.append(NTL::conv<NTL::ZZ_p>(pair.first));
      values_server.append(NTL::conv<NTL::ZZ_p>(pair.second));
    }

    // setup encryption
    if(key.size() == 0) {
      key.resize(block_size);
      gcry_randomize(key.data(), block_size, GCRY_STRONG_RANDOM);
    }
    gcry_cipher_hd_t handle;
    gcry_cipher_open(&handle, cipher, GCRY_CIPHER_MODE_CTR, 0);
    gcry_cipher_setkey(handle, key.data(), block_size);

    // encrypt server values
    for(size_t i = 0; i < server_in.size(); i++) {
      NTL::ZZ val;
      NTL::conv(val, values_server[i]);
      val <<= statistical_security;
      // use AES counter mode with the element as the counter
      unsigned char buf[block_size] = {0};
      unsigned char ctr[block_size] = {0};
      NTL::BytesFromZZ(ctr, (NTL::conv<NTL::ZZ>(elements_server[i]) << 8 * (sizeof(nonce))) + nonce, block_size);
      gcry_cipher_setctr(handle, ctr, block_size);
      if(NTL::NumBytes(val) > block_size) {
        BOOST_THROW_EXCEPTION(
          std::runtime_error("Server value does not fit in plaintext space"));
      }
      NTL::BytesFromZZ(buf, val, block_size);
      gcry_cipher_encrypt(handle, buf, block_size, nullptr, 0);
      NTL::conv(values_server[i], NTL::ZZFromBytes(buf, block_size));
    }

    // interpolate polynomial over the values
    // interpolate_recursive(poly_server, elements_server,values_server);
    poly_interpolate_zp_recursive(values_server.length() - 1,
      elements_server.data(), values_server.data(), poly_server);
    size_t num_elements_client;
    chan.recv(num_elements_client);
    chan.send(poly_server);

    // set up inputs for obliv-c
    std::vector<uint8_t> result(num_elements_client * sizeof(V));
    pir_poly_oblivc_args args = {
      .statistical_security = statistical_security,
      .value_type_size = sizeof(V),
      .input_size = key.size(),
      .input = key.data(),
      .result = result.data()
    };

    // run yao's protocol using Obliv-C
    ProtocolDesc pd;
    chan.flush();
    if(chan.connect_to_oblivc(pd) == -1) {
      BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
    }
    setCurrentParty(&pd, 1);
    execYaoProtocol(&pd, pir_poly_oblivc, &args);
    cleanupProtocol(&pd);

    server_out.resize(num_elements_client);
    for(size_t i = 0; i < server_out.size(); i++) {
      server_out[i] = 0;
      for(size_t j = 0; j < sizeof(V); j++) {
        server_out[i] |= (V(result[i*sizeof(V)+j]) << (8 * j));
      }
    }
  } catch (NTL::ErrorObject& ex) {
    BOOST_THROW_EXCEPTION(ex);
  }
}

template<typename K, typename V>
void pir_protocol_poly<K, V>::run_client(const std::vector<K>& client_in, std::vector<V>& client_out) {
  try {
    NTL::ZZ_pPush push(modulus);
    nonce++;

    // receive polynomial from server and send number of inputs
    NTL::ZZ_pX poly_server;
    chan.send(client_in.size());
    // we cannot use send_recv because modulus for NTL::ZZ_p would need to be
    // installed in the thread created by send_recv
    chan.recv(poly_server);

    // convert client inputs to NTL vectors
    NTL::Vec<NTL::ZZ_p> elements_client;
    NTL::Vec<NTL::ZZ_p> values_client;
    elements_client.SetLength(client_in.size());
    values_client.SetLength(client_in.size());
    std::copy(client_in.begin(), client_in.end(), elements_client.begin());
    // evaluate polynomial using fastpoly
    // evaluate_recursive(values_client, poly_server, elements_client);
    poly_evaluate_zp_recursive(elements_client.length() - 1, poly_server,
      elements_client.data(), values_client.data());

    std::vector<uint8_t> result(values_client.length() * sizeof(V));
    // set up inputs for obliv-c
    std::vector<uint8_t> ciphertexts_client(values_client.length() * 2 * block_size);
    pir_poly_oblivc_args args = {
      .statistical_security = statistical_security,
      .value_type_size = sizeof(V),
      .input_size = ciphertexts_client.size(),
      .input = ciphertexts_client.data(),
      .result = result.data()
    };
    // serialize ciphertexts and elements (used as ctr in decryption)
    for(size_t i = 0; i < values_client.length(); i++) {
      NTL::BytesFromZZ(ciphertexts_client.data() + i*2*block_size,
        NTL::conv<NTL::ZZ>(values_client[i]), block_size);
      NTL::BytesFromZZ(ciphertexts_client.data() + (i*2+1)*block_size,
        (NTL::conv<NTL::ZZ>(elements_client[i]) << 8 * (sizeof(nonce))) + nonce, block_size);
    }

    // run yao's protocol using Obliv-C
    ProtocolDesc pd;
    chan.flush();
    if(chan.connect_to_oblivc(pd) == -1) {
      BOOST_THROW_EXCEPTION(std::runtime_error("run_client: connection failed"));
    }
    setCurrentParty(&pd, 2);
    execYaoProtocol(&pd, pir_poly_oblivc, &args);
    cleanupProtocol(&pd);

    client_out.resize(values_client.length());
    for(size_t i = 0; i < client_out.size(); i++) {
      client_out[i] = 0;
      for(size_t j = 0; j < sizeof(V); j++) {
        client_out[i] |= (V(result[i*sizeof(V)+j]) << (8 * j));
      }
    }
  } catch (NTL::ErrorObject& ex) {
    BOOST_THROW_EXCEPTION(ex);
  }
}
