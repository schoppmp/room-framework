#pragma once

#include <numeric>
#include "NTL/ZZ.h"
#include "NTL/ZZ_pX.h"
#include "NTL/vector.h"
#include "gcrypt.h"
#include "mpc_utils/comm_channel.hpp"
#include "sparse_linear_algebra/oblivious_map/oblivious_map.hpp"

extern "C" {
void gcryDefaultLibInit();  // defined in Obliv-C, but not in obliv.h
}

template <typename K, typename V>
class basic_oblivious_map : public virtual oblivious_map<K, V> {
 private:
  const int cipher;
  const size_t block_size;
  std::vector<uint8_t> key;
  comm_channel& chan;

 public:
  basic_oblivious_map(comm_channel& chan)
      : oblivious_map<K, V>(),
        cipher(GCRY_CIPHER_AES128),
        block_size(16),
        chan(chan) {
    // initialize libgcrypt via obliv-c
    gcryDefaultLibInit();
    // check if sizes fit into ciphertexts
    // TODO: implement encryption of multiple blocks for really large value
    // types?
    if (block_size < sizeof(V) + 1 || block_size < sizeof(K) + 1) {
      BOOST_THROW_EXCEPTION(
          std::invalid_argument("Block size too small for given types"));
    }
  }
  ~basic_oblivious_map() {}

  using pair_range = typename oblivious_map<K, V>::pair_range;
  using key_range = typename oblivious_map<K, V>::key_range;
  using value_range = typename oblivious_map<K, V>::value_range;

  void run_server(const pair_range input, const value_range defaults,
                  bool shared_output,
                  mpc_utils::Benchmarker* benchmarker = nullptr);
  void run_client(const key_range input, value_range output, bool shared_output,
                  mpc_utils::Benchmarker* benchmarker = nullptr);
};

#include "basic_oblivious_map.tpp"
