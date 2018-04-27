#include <numeric>
#include <map>
#include <chrono>
#include <random>
#include <boost/serialization/map.hpp>
#include <NTL/vector.h>

#include "fastpoly/recursive.h"
#include "mpc-utils/boost_serialization.hpp"
#include "mpc-utils/mpc_config.hpp"
#include "mpc-utils/party.hpp"
#include "pir_protocol_poly.hpp"
#include "pir_protocol_fss.hpp"
// #include "pir_protocol_scs.hpp"

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
    if(pir_type != "poly" && pir_type != "fss" && pir_type != "scs") {
      BOOST_THROW_EXCEPTION(po::error("'pir_type' must be either `poly` or `fss`"));
    }
    mpc_config::validate();
  }
public:
  ssize_t num_elements_server;
  ssize_t num_elements_client;
  int16_t statistical_security;
  std::string pir_type;

  test_pir_config() {
    namespace po = boost::program_options;
    add_options()
      ("num_elements_server,N", po::value(&num_elements_server)->required(), "Number of non-zero elements in the server's database")
      ("num_elements_client,n", po::value(&num_elements_client)->required(), "Number of non-zero elements in the client's database")
      ("statistical_security,s", po::value(&statistical_security)->default_value(40), "Statistical security parameter")
      ("pir_type", po::value(&pir_type)->required(), "PIR type: poly | fss | scs");
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
  using value_type = uint32_t;
  try {
    std::unique_ptr<pir_protocol<key_type, value_type>> proto;
    if(conf.pir_type == "poly") {
      proto = std::unique_ptr<pir_protocol<key_type, value_type>>(
        new pir_protocol_poly<key_type, value_type>(chan, conf.statistical_security));
    } else if(conf.pir_type == "fss") {
      proto = std::unique_ptr<pir_protocol<key_type, value_type>>(
        new pir_protocol_fss<key_type, value_type>(chan));
    // } else if(conf.pir_type == "scs") {
    //   proto = std::unique_ptr<pir_protocol<key_type, value_type>>(
    //     new pir_protocol_scs<key_type, value_type>(chan));
    }
    if(party.get_id() == 0) {
      // use primes as inputs for easy recognition
      NTL::PrimeSeq primes;
      // map from keys to values
      std::map<key_type, value_type> server_in;
      // // alternative approach: save keys and values in two vectors
      // std::vector<key_type> server_keys_in(conf.num_elements_server);
      // std::vector<value_type> server_values_in(conf.num_elements_server);
      for(size_t i = 0; i < conf.num_elements_server; i++) {
        key_type current_key = 2*i + 42;
        value_type current_value = primes.next();
        server_in[current_key] = current_value;
        // server_keys_in[i] = current_key;
        // server_values_in[i] = current_value;
      }
      std::vector<value_type> defaults(conf.num_elements_client);
      // generate random default values
      std::uniform_int_distribution<value_type> dist;
      std::random_device r;
      std::generate(defaults.begin(), defaults.end(), [&]{return dist(r);});
      // run PIR protocol
      benchmark([&]() {
        proto->run_server(server_in.begin(), server_in.size(), defaults.begin(),
          defaults.size());
        // proto->run_server(server_keys_in.begin(), server_values_in.begin(),
        //   server_in.size(), defaults.begin(), defaults.size());
      }, "PIR Protocol (Server)");

      // send result for testing
      chan.send(defaults);
      chan.send(server_in);
    } else {
      // generate client elements
      std::vector<key_type> client_in(conf.num_elements_client);
      std::iota(client_in.begin(), client_in.end(), 23);

      // run PIR protocol
      std::vector<value_type> result(client_in.size());
      benchmark([&]() {
        proto->run_client(client_in.begin(), result.begin(), client_in.size());
      }, "PIR Protocol (Client)");

      // check correctness of the result
      std::map<key_type, value_type> server_in;
      std::vector<value_type> result_server;
      chan.recv(result_server);
      chan.recv(server_in);
      for(size_t i = 0; i < result.size(); i++) {
        value_type expected;
        try {
          expected = server_in.at(client_in[i]);
        } catch (std::out_of_range& e) {
          expected = result_server[i];
        }
        if(expected != result[i]) {
          std::cerr << "Expected " << expected << "\nGot " << result[i] << "\n";
          BOOST_THROW_EXCEPTION(
            std::runtime_error("Value of PIR result does not match"));
        }
      }
    }
  } catch(boost::exception &ex) {
    std::cerr << boost::diagnostic_information(ex);
    return 1;
  }
}
