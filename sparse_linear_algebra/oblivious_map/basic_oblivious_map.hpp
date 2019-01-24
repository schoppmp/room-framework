#pragma once

#include <numeric>
#include "external/mpc_utils/third_party/ntl/ntl/include/NTL/vector.h"
#include "external/mpc_utils/third_party/ntl/ntl/include/NTL/ZZ.h"
#include "external/mpc_utils/third_party/ntl/ntl/include/NTL/ZZ_pX.h"
#include <gcrypt.h>

#include "oblivious_map.hpp"
#include "mpc_utils/comm_channel.hpp"

extern "C" {
  void gcryDefaultLibInit(); // defined in Obliv-C, but not in obliv.h
}

template<typename K, typename V>
class basic_oblivious_map : public virtual oblivious_map<K,V> {
private:
  const int cipher;
  const size_t block_size;
  std::vector<uint8_t> key;
  comm_channel& chan;
  bool print_times;

public:
  basic_oblivious_map(
    comm_channel& chan,
    bool print_times = false
  ) :
    oblivious_map<K,V>(),
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
  ~basic_oblivious_map() { }

  using pair_range = typename oblivious_map<K, V>::pair_range;
  using key_range = typename oblivious_map<K, V>::key_range;
  using value_range = typename oblivious_map<K, V>::value_range;

  void run_server(const pair_range input, const value_range defaults,
    bool shared_output);
  void run_client(const key_range input, value_range output,
    bool shared_output);
};

#include "basic_oblivious_map.tpp"
