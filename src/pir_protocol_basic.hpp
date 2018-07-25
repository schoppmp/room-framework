#pragma once

#include <numeric>
#include <NTL/vector.h>
#include <NTL/ZZ.h>
#include <NTL/ZZ_pX.h>
#include <gcrypt.h>

#include "pir_protocol.hpp"
#include "mpc-utils/comm_channel.hpp"

extern "C" {
  void gcryDefaultLibInit(); // defined in Obliv-C, but not in obliv.h
}

template<typename K, typename V>
class pir_protocol_basic : public virtual pir_protocol<K,V> {
private:
  const int cipher;
  const size_t block_size;
  std::vector<uint8_t> key;
  comm_channel& chan;
  bool print_times;

public:
  pir_protocol_basic(
    comm_channel& chan,
    bool print_times = false
  ) :
    pir_protocol<K,V>(),
    cipher(GCRY_CIPHER_AES128), block_size(16),
    chan(chan), print_times(print_times)
  {
    // initialize libgcrypt via obliv-c
    gcryDefaultLibInit();
    // check if sizes fit into ciphertexts
    // TODO: implement encryption of multiple blocks for really large value types?
    if(block_size < sizeof(V) + 1 ||
      block_size < sizeof(K) + 1)
    {
      BOOST_THROW_EXCEPTION(std::invalid_argument(
        "Block size too small for given types"));
    }
  }
  ~pir_protocol_basic() { }

  using pair_range = typename pir_protocol<K, V>::pair_range;
  using key_range = typename pir_protocol<K, V>::key_range;
  using value_range = typename pir_protocol<K, V>::value_range;

  void run_server(const pair_range input, const value_range defaults,
    bool shared_output);
  void run_client(const key_range input, value_range output,
    bool shared_output);
};

#include "pir_protocol_basic.tpp"
