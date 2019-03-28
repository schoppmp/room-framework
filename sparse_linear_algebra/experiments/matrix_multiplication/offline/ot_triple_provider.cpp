#include "sparse_linear_algebra/matrix_multiplication/offline/ot_triple_provider.hpp"
#include "mpc_utils/benchmarker.hpp"
#include "mpc_utils/mpc_config.hpp"
#include "mpc_utils/party.hpp"
#include "sparse_linear_algebra/util/get_ceil.hpp"

class OtTripleProviderConfig : public virtual mpc_config {
 protected:
  void validate() {
    namespace po = boost::program_options;
    if (party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
    }
    if (rows_server.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'rows_server' must be passed at least once"));
    }
    for (auto l : rows_server) {
      if (l <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'rows_server' must be positive"));
      }
    }
    if (inner_dim.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'inner_dim' must be passed at least once"));
    }
    for (auto m : inner_dim) {
      if (m <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'inner_dim' must be positive"));
      }
    }
    if (cols_client.size() == 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'cols_client' must be passed at least once"));
    }
    for (auto n : cols_client) {
      if (n <= 0) {
        BOOST_THROW_EXCEPTION(po::error("'cols_client' must be positive"));
      }
    }
    for (const auto& type : triple_type) {
      if (type != "distributed" && type != "shared") {
        BOOST_THROW_EXCEPTION(
            po::error("'triply_type' must be 'distributed' or 'shared'"));
      }
    }
    mpc_config::validate();
  }

 public:
  std::vector<int> rows_server;
  std::vector<int> inner_dim;
  std::vector<int> cols_client;
  std::vector<std::string> triple_type;
  int max_runs;
  bool measure_communication;

  OtTripleProviderConfig() : mpc_config() {
    namespace po = boost::program_options;
    add_options()(
        "rows_server,l", po::value(&rows_server)->composing(),
        "Number of rows in the server's matrix; can be passed multiple times")(
        "inner_dim,m", po::value(&inner_dim)->composing(),
        "Number of columns in A and number of rows in B; can be passed "
        "multiple times")("cols_client,n", po::value(&cols_client)->composing(),
                          "Number of columns in the client's matrix; can be "
                          "passed multiple times")(
        "max_runs", po::value(&max_runs)->default_value(-1),
        "Maximum number of runs. Default is unlimited")(
        "triple_type", po::value(&triple_type)->composing(),
        "Type of the triples generated ('distributed' or 'shared'). Can be "
        "passed multiple times.")(
        "measure_communication",
        po::bool_switch(&measure_communication)->default_value(false),
        "Measure communication");
  }
};

int main(int argc, const char* argv[]) {
  using sparse_linear_algebra::matrix_multiplication::offline::OTTripleProvider;
  using T = uint64_t;
  // parse config
  OtTripleProviderConfig conf;
  try {
    conf.parse(argc, argv);
  } catch (boost::program_options::error& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  int role = conf.party_id;
  emp::NetIO io(!role ? nullptr : conf.servers[0].host.c_str(),
                conf.servers[0].port);

  int num_experiments =
      std::max({conf.rows_server.size(), conf.inner_dim.size(),
                conf.cols_client.size()});

  for (int num_runs = 0; conf.max_runs < 0 || num_runs < conf.max_runs;
       num_runs++) {
    for (int experiment = 0; experiment < num_experiments; experiment++) {
      std::cout << "Run " << num_runs << "\n";
      int l = get_ceil(conf.rows_server, experiment);
      int m = get_ceil(conf.inner_dim, experiment);
      int n = get_ceil(conf.cols_client, experiment);
      const std::string& triple_type = get_ceil(conf.triple_type, experiment);

      std::cout << "l = " << l << "\nm = " << m << "\nn = " << n
                << "\ntriple_type = " << triple_type << "\n";

      mpc_utils::Benchmarker benchmarker;
      benchmarker.BenchmarkFunction("Triple Generation", [&] {
        if (triple_type == "shared") {
          OTTripleProvider<uint64_t, true> triples(l, m, n, role, &io);
          triples.Precompute(1);
        } else {
          OTTripleProvider<uint64_t, false> triples(l, m, n, role, &io);
          triples.Precompute(1);
        }
      });
      for (const auto& pair : benchmarker.GetAll()) {
        std::cout << pair.first << ": " << pair.second << "\n";
      }
    }
  }
}