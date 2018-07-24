#pragma once
extern "C" {
  #include "obliv_common.h"
  #include "zero_sharing.h"
}
#include "mpc-utils/comm_channel.hpp"
#include "util/serialize_le.hpp"
#include "fastpoly/recursive.h"
#include <Eigen/Dense>
#include <random>
#include <gcrypt.h>
#include <boost/exception/all.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/serialization/vector.hpp>
#include <NTL/vector.h>
#include <NTL/ZZ.h>
#include <NTL/ZZ_pX.h>

// Inputs:
// Server - A share of a short vector v of length l
//        - A mapping I of all [l] positions to indexes in [n]
// Client - A second share of v
//
// Outputs:
// Both parties: Shares of a vector v' of length n with values from v at
//               indexes from I and zeros everywhere else

template<typename T>
std::vector<T> zero_sharing_server(
  std::vector<T> v,
  std::vector<size_t> I,
  size_t n,
  comm_channel& chan
) {
  size_t l = v.size();
  if(I.size() != v.size()) {
    BOOST_THROW_EXCEPTION(std::runtime_error("v and I need to have the same length"));
  }
  // sort I but remember original positions
  std::vector<std::pair<size_t, size_t>> I_order(l);
  for(size_t i = 0; i < l; i++) {
    I_order[i] = std::make_pair(I[i], i);
  }
  boost::sort(I_order);
  // set up NTL and gcrypt
  NTL::ZZ modulus((NTL::ZZ(1) << 128) - 159);
  NTL::ZZ_pPush push(modulus);
  gcryDefaultLibInit();
  auto cipher = GCRY_CIPHER_AES128;
  gcry_cipher_hd_t handle;
  dhRandomInit();
  const size_t block_size = 16;
  const size_t element_size = sizeof(T) + block_size;
  // set up OT arguments
  bool *choices = (bool *) calloc(n, sizeof(bool));
  for(auto i : I) {
    choices[i] = 1;
  }
  std::vector<uint8_t> ot_result(element_size * n);
  // receive key shares at positions in I, result shares at positions not in I
  ProtocolDesc pd;
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 1);
  auto ot = honestOTExtRecverNew(&pd, 0);
  honestOTExtRecv1Of2(ot, reinterpret_cast<char *>(ot_result.data()), choices,
    n, element_size);
  honestOTExtRecverRelease(ot);
  // unpack
  std::vector<T> s(n);
  std::vector<T> t(l); // encrypted shares corresponding to indices in I
  NTL::Vec<NTL::ZZ_p> share_K;
  NTL::Vec<NTL::ZZ_p> interpolate_pos;
  share_K.SetLength(l);
  interpolate_pos.SetLength(l);
  size_t num_shares = 0;
  for(size_t i = 0; i < n; i++) {
    if(choices[i]) {
      deserialize_le(&t[I_order[num_shares].second], &ot_result[i * element_size], 1);
      NTL::conv(share_K[num_shares], NTL::ZZFromBytes(
        &ot_result[i * element_size + sizeof(T)], block_size));
      NTL::conv(interpolate_pos[num_shares], i+1);
      num_shares++;
    } else {
      deserialize_le(&s[i], &ot_result[i * element_size], 1);
    }
  }
  // combine shares to get Key
  NTL::ZZ_pX poly;
  poly_interpolate_zp_recursive(l - 1, interpolate_pos.data(), share_K.data(),
    poly);
  // std::cout << "interpolate_pos = " << interpolate_pos << "\n";
  // std::cout << "share_K = " << share_K << "\n";
  // std::cout << "poly = " << poly << "\n";
  std::vector<uint8_t> K(block_size);
  NTL::BytesFromZZ(K.data(), NTL::conv<NTL::ZZ>(NTL::ConstTerm(poly)), block_size);
  // decrypt shares not in I
  gcry_cipher_open(&handle, cipher, GCRY_CIPHER_MODE_CTR, 0);
  gcry_cipher_setkey(handle, K.data(), block_size);
  for(size_t i = 0; i < n; i++) {
    if(choices[i]) {
      continue;
    }
    uint8_t buf[block_size] = {0};
    uint8_t ctr[block_size] = {0};
    serialize_le(&ctr[0], &i, 1);
    gcry_cipher_setctr(handle, ctr, block_size);
    gcry_cipher_encrypt(handle, buf, block_size, nullptr, 0);
    T r;
    deserialize_le(&r, buf, 1);
    s[i] = -(s[i] ^ r);
  }
  gcry_cipher_close(handle);

  // compute shares of nonzero values in yao protocol
  std::vector<uint8_t> indexes_server_bytes(sizeof(size_t) * l);
  std::vector<uint8_t> ciphertexts_server_bytes(sizeof(T) * l);
  std::vector<uint8_t> values_server_bytes(sizeof(T) * l);
  std::vector<uint8_t> result_server_bytes(sizeof(T) * l);
  serialize_le(indexes_server_bytes.begin(), I.begin(), l);
  serialize_le(ciphertexts_server_bytes.begin(), t.begin(), l);
  serialize_le(values_server_bytes.begin(), v.begin(), l);
  zero_sharing_oblivc_args args = {
    .element_size = sizeof(T),
    .num_ciphertexts = l,
    .indexes_server = indexes_server_bytes.data(),
    .values = values_server_bytes.data(),
    .ciphertexts_server = ciphertexts_server_bytes.data(),
    .key_client = nullptr,
    .result_server = result_server_bytes.data(),
  };
  execYaoProtocol(&pd, zero_sharing_oblivc, &args);

  // copy computed shares to their indexes
  for(size_t i = 0; i < l; i++) {
    deserialize_le(&s[I[i]], &result_server_bytes[i * sizeof(T)], 1);
  }

  free(choices);
  cleanupProtocol(&pd);
  return s;
}

template<typename T>
std::vector<T> zero_sharing_client(
  std::vector<T> v,
  size_t n,
  comm_channel& chan
) {
  size_t l = v.size();
  // setup encryption and generate keys
  const size_t block_size = 16;
  gcryDefaultLibInit();
  auto cipher = GCRY_CIPHER_AES128;
  gcry_cipher_hd_t handle;
  std::vector<uint8_t> K(block_size);
  std::vector<uint8_t> K2(block_size);
  gcry_randomize(K.data(), block_size, GCRY_STRONG_RANDOM);
  gcry_randomize(K2.data(), block_size, GCRY_STRONG_RANDOM);
  gcry_cipher_open(&handle, cipher, GCRY_CIPHER_MODE_CTR, 0);
  // generate seeds
  std::vector<T> r(n);
  gcry_randomize(r.data(), n * sizeof(T), GCRY_STRONG_RANDOM);
  // encrypt to get our own shares
  gcry_cipher_setkey(handle, K.data(), block_size);
  std::vector<T> s(n);
  for(size_t i = 0; i < n; i++) {
    uint8_t buf[block_size] = {0};
    uint8_t ctr[block_size] = {0};
    serialize_le(&ctr[0], &i, 1);
    gcry_cipher_setctr(handle, ctr, block_size);
    gcry_cipher_encrypt(handle, buf, block_size, nullptr, 0);
    deserialize_le(&s[i], buf, 1);
    s[i] ^= r[i];
  }
  // encrypt again under second key
  gcry_cipher_setkey(handle, K2.data(), block_size);
  std::vector<T> t(n);
  for(size_t i = 0; i < n; i++) {
    uint8_t buf[block_size] = {0};
    uint8_t ctr[block_size] = {0};
    serialize_le(&ctr[0], &i, 1);
    gcry_cipher_setctr(handle, ctr, block_size);
    gcry_cipher_encrypt(handle, buf, block_size, nullptr, 0);
    deserialize_le(&t[i], buf, 1);
    t[i] ^= s[i];
  }
  gcry_cipher_close(handle);
  // secret-share K
  NTL::ZZ modulus((NTL::ZZ(1) << 128) - 159);
  NTL::ZZ_pPush push(modulus);
  NTL::Vec<NTL::ZZ_p> share_K;
  NTL::Vec<NTL::ZZ_p> eval_pos; // where the polynomial will be evaluated
  share_K.SetLength(n);
  eval_pos.SetLength(n);
  std::iota(eval_pos.begin(), eval_pos.end(), 1);
  auto poly = NTL::random_ZZ_pX(l); // degree-(l-1) polynomial
  if(NTL::deg(poly) != l - 1) {
    std::cerr << "Have deg(poly) = " << NTL::deg(poly) << ", want " << l-1 << "\n";
    BOOST_THROW_EXCEPTION(std::runtime_error("FIXME"));
  }
  NTL::ZZ_p K_coeff;
  NTL::conv(K_coeff, NTL::ZZFromBytes(K.data(), block_size));
  NTL::SetCoeff(poly, 0, K_coeff);
  poly_evaluate_zp_recursive(n - 1, poly, eval_pos.data(), share_K.data());
  // std::cout << "eval_pos = " << eval_pos << "\n";
  // std::cout << "share_K = " << share_K << "\n";
  // std::cout << "poly = " << poly << "\n";
  // set up OT arguments (we are the sender)
  const size_t element_size = sizeof(T) + block_size; // one element of t + one share of K
  std::vector<uint8_t> opt0(element_size * n, 0);
  std::vector<uint8_t> opt1(element_size * n, 0);
  for(size_t i = 0; i < n; i++) {
    serialize_le(&opt0[i * element_size], &r[i], 1);
    serialize_le(&opt1[i * element_size], &t[i], 1);
    NTL::BytesFromZZ(&opt1[i * element_size + sizeof(T)],
      NTL::conv<NTL::ZZ>(share_K[i]), block_size);
  }
  // run OT extension
  dhRandomInit();
  ProtocolDesc pd;
  if(chan.connect_to_oblivc(pd) == -1) {
    BOOST_THROW_EXCEPTION(std::runtime_error("run_server: connection failed"));
  }
  setCurrentParty(&pd, 2);
  auto ot = honestOTExtSenderNew(&pd, 0);
  honestOTExtSend1Of2(ot, reinterpret_cast<char*>(opt0.data()),
    reinterpret_cast<char*>(opt1.data()), n, element_size);
  honestOTExtSenderRelease(ot);
  // run yao protocol to generate server's shares
  std::vector<uint8_t> values_client_bytes(sizeof(T) * l);
  serialize_le(values_client_bytes.begin(), v.begin(), l);
  zero_sharing_oblivc_args args = {
    .element_size = sizeof(T),
    .num_ciphertexts = l,
    .indexes_server = nullptr,
    .values = values_client_bytes.data(),
    .ciphertexts_server = nullptr,
    .key_client = K2.data(),
    .result_server = nullptr,
  };
  execYaoProtocol(&pd, zero_sharing_oblivc, &args);

  cleanupProtocol(&pd);
  return s;
}
