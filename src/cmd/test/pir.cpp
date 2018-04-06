#include "fastpoly/recursive.h"
#include "mpc-utils/mpc_config.hpp"
#include "mpc-utils/party.hpp"
#include "mpc-utils/boost_serialization.hpp"
#include <NTL/vector.h>
#include <numeric>
#include <chrono>
#include <gcrypt.h>

// used for time measurements
template<class F>
void benchmark(F f, const std::string& label) {
  auto start = std::chrono::steady_clock::now();
  f();
  std::chrono::duration<double> d = std::chrono::steady_clock::now() - start;
  std::cout << label << ": " << d.count() << "s\n";
}

// NTL-style interface for recursive interpolation
void interpolate_recursive(
  NTL::ZZ_pX& f,
  const NTL::vec_ZZ_p& a,
  const NTL::vec_ZZ_p& b
) {
  poly_interpolate_zp_recursive(std::min(a.length(), b.length()) - 1,
    a.data(), b.data(), f);
}
NTL::ZZ_pX interpolate_recursive(
  const NTL::vec_ZZ_p& a,
  const NTL::vec_ZZ_p& b
) {
  NTL::ZZ_pX f;
  interpolate_recursive(f, a, b);
  return f;
}
void evaluate_recursive(
  NTL::vec_ZZ_p& b,
  const NTL::ZZ_pX& f,
  const NTL::vec_ZZ_p& a
) {
  b.SetLength(a.length());
  poly_evaluate_zp_recursive(NTL::deg(f), f, a.data(), b.data());
}
NTL::vec_ZZ_p evaluate_recursive(
  const NTL::ZZ_pX& f,
  const NTL::vec_ZZ_p& a
) {
  NTL::vec_ZZ_p b;
  evaluate_recursive(b, f, a);
  return b;
}


class test_pir_config : public virtual mpc_config {
protected:
  void validate() {
    namespace po = boost::program_options;
    if(party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
    }
    if(num_elements_server <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'num_elements_server' must be positive"));
    }
    if(num_elements_client <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'num_elements_client' must be positive"));
    }
    if(statistical_security <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'statistical_security' must be positive"));
    }
    mpc_config::validate();
  }
public:
  ssize_t num_elements_server;
  ssize_t num_elements_client;
  int16_t statistical_security;

  test_pir_config() {
    namespace po = boost::program_options;
    add_options()
      ("num_elements_server,N", po::value(&num_elements_server)->required(), "Number of non-zero elements in the server's database")
      ("num_elements_client,n", po::value(&num_elements_client)->required(), "Number of non-zero elements in the client's database")
      ("statistical_security,s", po::value(&statistical_security)->default_value(40), "Statistical security parameter");
    set_default_filename("config/test/pir.ini");
  }
};

int main(int argc, const char **argv) {
  // parse config
  test_pir_config conf;
  try {
      conf.parse(argc, argv);
  } catch (boost::program_options::error &e) {
      std::cerr << e.what() << "\n";
      return 1;
  }
  party party(conf);
  auto chan = party.connect_to(1 - party.get_id());

  // initialize 128-bit prime field (order (2^128 - 159))
  NTL::ZZ_p::init((NTL::ZZ(1) << 128) - 159);

  NTL::ZZ_pX poly_server;
  NTL::Vec<NTL::ZZ_p> elements_server;
  elements_server.SetLength(conf.num_elements_server);
  std::iota(elements_server.begin(), elements_server.end(), 0);
  if(party.get_id() == 0) {
    // generate server values (just take the first N primes for now)
    NTL::Vec<NTL::ZZ_p> values_server(elements_server);
    NTL::PrimeSeq primes;
    std::generate(
      values_server.begin(),
      values_server.end(),
      [&]() { return primes.next(); }
    );

    // encrypt server values
    benchmark(
      [&]{
        gcry_cipher_hd_t handle;
        size_t blockSize = gcry_cipher_get_algo_keylen(GCRY_CIPHER_AES128);
        char key[blockSize];
        gcry_randomize(key, blockSize, GCRY_STRONG_RANDOM);
        gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
        gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
        gcry_cipher_open(&handle, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_ECB, 0);
        gcry_cipher_setkey(handle, key, blockSize);
        for(size_t i = 0; i < values_server.length(); i++) {
          uint32_t counter(i);
          unsigned char buf[blockSize] = {0};
          NTL::ZZ el;
          NTL::conv(el, values_server[i]);
          // TODO: we should be using "real" AES counter mode with a nonce
          el <<= (conf.statistical_security + 8 * sizeof(counter));
          if(NTL::NumBytes(el) > blockSize) {
            BOOST_THROW_EXCEPTION(
              std::runtime_error("Server value does not fit in plaintext space"));
          }
          NTL::BytesFromZZ(buf, el, blockSize);
          gcry_cipher_encrypt(handle, buf, blockSize, nullptr, 0);
          NTL::conv(values_server[i], NTL::ZZFromBytes(buf, blockSize));
        }
      }, "Encryption");
    benchmark(
      [&]{interpolate_recursive(poly_server, elements_server,values_server);},
      "Interpolation"
    );
    chan.send(poly_server);
  } else {
    chan.recv(poly_server);
    NTL::Vec<NTL::ZZ_p> values_server;
    // evaluate polynomial using fastpoly
    benchmark(
      [&]{evaluate_recursive(values_server, poly_server, elements_server);},
      "Evaluation"
    );
  }
}
