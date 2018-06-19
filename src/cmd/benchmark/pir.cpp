#include <numeric>
#include <map>
#include <random>
#include <boost/serialization/map.hpp>
#include <NTL/vector.h>

#include "fastpoly/recursive.h"
#include "mpc-utils/boost_serialization/ntl.hpp"
#include "mpc-utils/mpc_config.hpp"
#include "mpc-utils/party.hpp"
#include "pir_protocol_poly.hpp"
#include "pir_protocol_fss.hpp"
#include "pir_protocol_scs.hpp"
#include "util/time.h"


class test_pir_config : public virtual mpc_config {
protected:
  void validate() {
    namespace po = boost::program_options;
    if(party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
    }
    if(max_elements_server <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'max_elements_server' must be positive"));
    }
    if(max_elements_client <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'max_elements_client' must be positive"));
    }
    if(statistical_security <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'statistical_security' must be positive"));
    }
    for(auto& pir_type : pir_types) {
      if(pir_type != "poly" && pir_type != "fss" && pir_type != "fss_cprg" && pir_type != "scs") {
      BOOST_THROW_EXCEPTION(po::error("'pir_type' must be either `poly`, `scs`, `fss` or `fss_cprg`"));
      }
    }
    mpc_config::validate();
  }
public:
  ssize_t max_elements_server;
  ssize_t max_elements_client;
  int16_t statistical_security;
  std::vector<std::string> pir_types;

  test_pir_config() {
    namespace po = boost::program_options;
    add_options()
      ("max_elements_server,N", po::value(&max_elements_server)->required(), "Number of non-zero elements in the server's database")
      ("max_elements_client,n", po::value(&max_elements_client)->required(), "Number of non-zero elements in the client's database")
      ("statistical_security,s", po::value(&statistical_security)->default_value(40), "Statistical security parameter")
      ("pir_type", po::value(&pir_types)->composing(), "PIR type: poly | fss | scs; can be passed multiple times");
    set_default_filename("config/benchmark/pir.ini");
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

  using key_type = uint32_t;
  using value_type = uint32_t;
  std::unique_ptr<pir_protocol<key_type, value_type>> proto;
  for(auto& pir_type : conf.pir_types) {
    try {
      if(pir_type == "poly") {
        proto = std::unique_ptr<pir_protocol<key_type, value_type>>(
          new pir_protocol_poly<key_type, value_type>(chan, conf.statistical_security, true));
      } else if(pir_type == "fss") {
        proto = std::unique_ptr<pir_protocol<key_type, value_type>>(
          new pir_protocol_fss<key_type, value_type>(chan, false, true));
      } else if(pir_type == "fss_cprg") {
        proto = std::unique_ptr<pir_protocol<key_type, value_type>>(
          new pir_protocol_fss<key_type, value_type>(chan, true, true));
      } else { // if(conf.pir_type == "scs") {
        proto = std::unique_ptr<pir_protocol<key_type, value_type>>(
          new pir_protocol_scs<key_type, value_type>(chan, true));
      }
    } catch(boost::exception &ex) {
      std::cerr << boost::diagnostic_information(ex);
      return 1;
    }
    std::cout << "PIR type: " << pir_type << "\n";
    for(size_t num_elements_server = 1;
      num_elements_server <= conf.max_elements_server; num_elements_server*=2
    ) {
      for(size_t num_elements_client = 1;
        num_elements_client <= conf.max_elements_client; num_elements_client*=2
      ) {
        try {
          std::cout << "num_elements_server: " << num_elements_server << "\n";
          std::cout << "num_elements_client: " << num_elements_client << "\n";
          if(party.get_id() == 0) {
            std::vector<key_type> server_keys_in(num_elements_server);
            std::iota(server_keys_in.begin(), server_keys_in.end(), 0);
            std::vector<value_type> server_values_in(num_elements_server, 23);
            std::vector<value_type> server_defaults(num_elements_client, 13);
            benchmark([&]() {
              proto->run_server(server_keys_in, server_values_in, server_defaults);
            }, "total_time");
          } else {
            std::vector<key_type> client_in(num_elements_client);
            std::iota(client_in.begin(), client_in.end(), 42);
            std::vector<value_type> client_out(num_elements_client);
            benchmark([&]() {
              proto->run_client(client_in, client_out);
            }, "total_time");
          }
        } catch(boost::exception &ex) {
          std::cerr << boost::diagnostic_information(ex);
          return 1;
        }
      }
    }
  }
}
