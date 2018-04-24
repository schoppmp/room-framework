#pragma once

#include <NTL/vector.h>
#include <numeric>
#include <gcrypt.h>

#include "pir_protocol.hpp"
#include "mpc-utils/comm_channel.hpp"

template<typename K, typename V>
class pir_protocol_poly : public virtual pir_protocol<K,V> {
private:
  NTL::ZZ modulus;
  uint16_t statistical_security;
  int cipher;
  size_t block_size;
  std::vector<uint8_t> key;
  uint64_t nonce; // increased with every protocol execution
  comm_channel& chan;

  static void init_gcrypt() {
  }
public:
  pir_protocol_poly(comm_channel& chan, uint16_t statistical_security) :
    pir_protocol<K,V>(),
    modulus((NTL::ZZ(1) << 128) - 159),
    statistical_security(statistical_security),
    cipher(GCRY_CIPHER_AES128), block_size(gcry_cipher_get_algo_keylen(cipher)),
    key(block_size), nonce(0), chan(chan)
  {
    // initialize libgcrypt
    gcry_check_version(nullptr);
    if(!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P)) {
      gcry_control(GCRYCTL_DISABLE_SECMEM);
      gcry_control(GCRYCTL_INITIALIZATION_FINISHED);
    }
    // generate key
    gcry_randomize(key.data(), block_size, GCRY_STRONG_RANDOM);
  }

  using pir_protocol<K, V>::run_server;
  using pir_protocol<K, V>::run_client;
  void run_server(const std::map<K,V>& server_in, std::vector<V>& server_out);
  void run_client(const std::vector<K>& client_in, std::vector<V>& client_out);
};

#include "pir_protocol_poly.tpp"
