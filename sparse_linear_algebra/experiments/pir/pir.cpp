#include <map>
#include <numeric>
#include <random>
#include "NTL/vector.h"
#include "boost/serialization/map.hpp"
#include "fastpoly/recursive.h"
#include "mpc_utils/boost_serialization/ntl.hpp"
#include "mpc_utils/mpc_config.hpp"
#include "mpc_utils/party.hpp"
#include "sparse_linear_algebra/oblivious_map/basic_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/poly_oblivious_map.hpp"
#include "sparse_linear_algebra/oblivious_map/sorting_oblivious_map.hpp"
#include "sparse_linear_algebra/util/get_ceil.hpp"
#include "sparse_linear_algebra/util/time.h"

class test_pir_config : public virtual mpc_config {
 protected:
  void validate() {
    namespace po = boost::program_options;
    if (party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
    }
    if (statistical_security <= 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'statistical_security' must be positive"));
    }
    // if(num_elements_client.size() != pir_types.size() ||
    // num_elements_server.size() != pir_types.size()) {
    //   BOOST_THROW_EXCEPTION(po::error("'num_elements_server', "
    //     "'num_elements_client', and 'pir_type' must be passed the same number
    //     " "of times"));
    // }
    for (auto N : num_elements_server) {
      if (N <= 0) {
        BOOST_THROW_EXCEPTION(
            po::error("'num_elements_server' must be positive"));
      }
    }
    for (auto n : num_elements_client) {
      if (n <= 0) {
        BOOST_THROW_EXCEPTION(
            po::error("'num_elements_client' must be positive"));
      }
    }
    for (auto &pir_type : pir_types) {
      if (pir_type != "basic" && pir_type != "poly" && pir_type != "scs") {
        BOOST_THROW_EXCEPTION(
            po::error("'pir_type' must be either "
                      "`basic`, `poly` or `scs`"));
      }
    }
    mpc_config::validate();
  }

 public:
  std::vector<ssize_t> num_elements_server;
  std::vector<ssize_t> num_elements_client;
  std::vector<std::string> pir_types;
  int16_t statistical_security;
  bool measure_communication;

  test_pir_config() {
    namespace po = boost::program_options;
    add_options()("num_elements_server,N",
                  po::value(&num_elements_server)->composing(),
                  "Number of non-zero elements in the server's database; can "
                  "be passed multiple times")(
        "num_elements_client,n", po::value(&num_elements_client)->composing(),
        "Number of non-zero elements in the client's database; can be passed "
        "multiple times")("pir_type", po::value(&pir_types)->composing(),
                          "PIR type: basic | poly | scs; can "
                          "be passed multiple times")(
        "statistical_security,s",
        po::value(&statistical_security)->default_value(40),
        "Statistical security parameter")(
        "measure_communication",
        po::bool_switch(&measure_communication)->default_value(false),
        "Measure communication");
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
  auto chan = party.connect_to(1 - party.get_id(), conf.measure_communication);

  using key_type = uint32_t;
  using value_type = uint32_t;
  std::unique_ptr<oblivious_map<key_type, value_type>> proto;
  // number of experiments is determined by the parameter passed the most times
  size_t num_experiments =
      std::max({conf.pir_types.size(), conf.num_elements_client.size(),
                conf.num_elements_server.size()});
  for (size_t num_runs = 0;; num_runs++) {
    std::cout << "Run " << num_runs << "\n";
    for (size_t experiment = 0; experiment < num_experiments; experiment++) {
      // if a parameter is passed less times than num_experiments, use the last
      // value for the remaining experiments
      size_t num_elements_server =
          get_ceil(conf.num_elements_server, experiment);
      size_t num_elements_client =
          get_ceil(conf.num_elements_client, experiment);
      auto &pir_type = get_ceil(conf.pir_types, experiment);
      try {
        if (pir_type == "basic") {
          proto = std::unique_ptr<oblivious_map<key_type, value_type>>(
              new basic_oblivious_map<key_type, value_type>(chan));
        } else if (pir_type == "poly") {
          proto = std::unique_ptr<oblivious_map<key_type, value_type>>(
              new poly_oblivious_map<key_type, value_type>(
                  chan, conf.statistical_security));
        } else {  // if(conf.pir_type == "scs") {
          proto = std::unique_ptr<oblivious_map<key_type, value_type>>(
              new sorting_oblivious_map<key_type, value_type>(chan));
        }
      } catch (boost::exception &ex) {
        std::cerr << boost::diagnostic_information(ex);
        return 1;
      }
      std::cout << "PIR type: " << pir_type << "\n";
      std::cout << "num_elements_server: " << num_elements_server << "\n";
      std::cout << "num_elements_client: " << num_elements_client << "\n";
      mpc_utils::Benchmarker benchmarker;
      try {
        if (party.get_id() == 0) {
          std::vector<key_type> server_keys_in(num_elements_server);
          std::iota(server_keys_in.begin(), server_keys_in.end(), 0);
          std::vector<value_type> server_values_in(num_elements_server, 23);
          std::vector<value_type> server_defaults(num_elements_client, 13);
          benchmarker.BenchmarkFunction("total_time", [&]() {
            proto->run_server(server_keys_in, server_values_in, server_defaults,
                              false, &benchmarker);
          });
        } else {
          std::vector<key_type> client_in(num_elements_client);
          std::iota(client_in.begin(), client_in.end(), 42);
          std::vector<value_type> client_out(num_elements_client);
          benchmarker.BenchmarkFunction("total_time", [&]() {
            proto->run_client(client_in, client_out, false, &benchmarker);
          });
        }
      } catch (boost::exception &ex) {
        std::cerr << boost::diagnostic_information(ex);
        return 1;
      }

      if (conf.measure_communication) {
        benchmarker.AddAmount("Bytes Sent (direct)", chan.get_num_bytes_sent());
      }
      for (const auto &pair : benchmarker.GetAll()) {
        std::cout << pair.first << ": " << pair.second << "\n";
      }
    }
  }
}
