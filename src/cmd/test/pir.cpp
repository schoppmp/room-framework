#include "fastpoly/recursive.h"
#include "mpc-utils/mpc_config.hpp"
#include "mpc-utils/party.hpp"
#include "mpc-utils/boost_serialization.hpp"
#include "pir_protocol_poly.hpp"
#include "pir_protocol_fss.hpp"
#include <NTL/vector.h>
#include <numeric>
#include <map>
#include <chrono>

// used for time measurements
template<class F>
void benchmark(F f, const std::string& label) {
  auto start = std::chrono::steady_clock::now();
  f();
  std::chrono::duration<double> d = std::chrono::steady_clock::now() - start;
  std::cout << label << ": " << d.count() << "s\n";
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

  using key_type = uint64_t;
  using value_type = uint16_t;
  try {
    // pir_protocol_poly<key_type, value_type> proto_impl(chan, conf.statistical_security);
    pir_protocol_fss<key_type, value_type> proto_impl(chan);

    pir_protocol<key_type, value_type>& proto = proto_impl;
    if(party.get_id() == 0) {
      // use primes as inputs for easy recognition
      NTL::PrimeSeq primes;
      std::map<key_type, value_type> server_in;
      for(size_t i = 0; i < conf.num_elements_server; i++) {
        server_in[i] = primes.next();
      }
      // run PIR protocol
      std::vector<value_type> result;
      benchmark([&]() {
        result = proto.run_server(server_in);
      }, "PIR Protocol (Server)");

      // send result for testing
      chan.send(result);
    } else {
      // generate client elements with fixed overlap for testing
      key_type overlap = key_type(std::min(ssize_t(100),
        std::min(conf.num_elements_server, conf.num_elements_client)));
      std::vector<key_type> client_in(conf.num_elements_client);
      std::iota(client_in.begin(), client_in.end(),
        key_type(conf.num_elements_server) - overlap);

      // run PIR protocol
      std::vector<value_type> result;
      benchmark([&]() {
        result = proto.run_client(client_in);
      }, "PIR Protocol (Client)");

      // add up values for testing
      std::vector<value_type> result_server;
      chan.recv(result_server);
      size_t count_matching = 0;
      for(size_t i = 0; i < result.size(); i++) {
        // Guess what the result type of adding two uint16_t is.
        // Correct! in C++ it is.... uint32_t -.- WTF?
        // https://stackoverflow.com/a/5563131
        // Anyway, that's the reason for the casts in the following condition.
        // TODO: switch to Rust ASAP!
        if(value_type(result[i] + result_server[i]) != value_type(0)) {
          count_matching++;
        }
      }

      if(count_matching != overlap) {
        std::cerr << "Expected " << overlap << "\nGot " << count_matching << "\n";
        BOOST_THROW_EXCEPTION(
          std::runtime_error("Intersection size does not match"));
      }
    }
  } catch(boost::exception &ex) {
    std::cerr << boost::diagnostic_information(ex);
    return 1;
  }
}
