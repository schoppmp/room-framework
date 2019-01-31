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
class poly_oblivious_map : public virtual oblivious_map<K, V> {
 private:
  const NTL::ZZ modulus;
  const uint16_t statistical_security;
  const int cipher;
  const size_t block_size;
  std::vector<uint8_t> key;
  uint64_t
      nonce;  // increased with every protocol execution;
              // currently unnecessary, since the key is regenerated as well
  comm_channel& chan;
  bool print_times;

 public:
  poly_oblivious_map(comm_channel& chan, uint16_t statistical_security,
                     bool print_times = false)
      : oblivious_map<K, V>(),
        modulus((NTL::ZZ(1) << 128) - 159),  // largest 128-bit prime
        statistical_security(statistical_security),
        cipher(GCRY_CIPHER_AES128),
        block_size(16),
        nonce(0),
        chan(chan),
        print_times(print_times) {
    // initialize libgcrypt via obliv-c
    gcryDefaultLibInit();
    // check if sizes fit into ciphertexts
    // TODO: implement encryption of multiple blocks for really large value
    // types?
    if (statistical_security % 8) {
      BOOST_THROW_EXCEPTION(
          std::invalid_argument("statistical_security must be divisible by 8"));
    }
    if (8 * block_size < 8 * sizeof(V) + statistical_security ||
        block_size < sizeof(nonce) + sizeof(K)) {
      BOOST_THROW_EXCEPTION(std::invalid_argument(
          "Block size too small for given types and statistical security"));
    }
  }
  ~poly_oblivious_map() {}

  using pair_range = typename oblivious_map<K, V>::pair_range;
  using key_range = typename oblivious_map<K, V>::key_range;
  using value_range = typename oblivious_map<K, V>::value_range;

  void run_server(const pair_range input, const value_range defaults,
                  bool shared_output,
                  mpc_utils::Benchmarker* benchmarker = nullptr);
  void run_client(const key_range input, value_range output,
                  bool shared_output,
                  mpc_utils::Benchmarker* benchmarker = nullptr);
};

#include "poly_oblivious_map.tpp"
