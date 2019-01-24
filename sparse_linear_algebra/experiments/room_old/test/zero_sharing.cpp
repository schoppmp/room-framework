#include "mpc_utils/comm_channel.hpp"
#include "mpc_utils/mpc_config.hpp"
#include "mpc_utils/party.hpp"

#include "sparse_linear_algebra/util/reservoir_sampling.hpp"
#include "sparse_linear_algebra/zero_sharing/zero_sharing.hpp"

class zero_sharing_config : public virtual mpc_config {
 protected:
  void validate() {
    namespace po = boost::program_options;
    if (party_id < 0 || party_id > 1) {
      BOOST_THROW_EXCEPTION(po::error("'party' must be 0 or 1"));
    }
    if (len <= 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'num_elements_server' must be positive"));
    }
    if (num_nonzeros <= 0) {
      BOOST_THROW_EXCEPTION(
          po::error("'num_elements_client' must be positive"));
    }
    if (len < num_nonzeros) {
      BOOST_THROW_EXCEPTION(po::error("'len' must be at least 'num_nonzeros'"));
    }
    mpc_config::validate();
  }

 public:
  ssize_t len;
  ssize_t num_nonzeros;
  std::string pir_type;

  zero_sharing_config() {
    namespace po = boost::program_options;
    add_options()("num_nonzeros,k", po::value(&num_nonzeros)->required(),
                  "Number of non-zero elements in the shared vector (= length "
                  "of the input vector)")(
        "len,n", po::value(&len)->required(),
        "Number elements in the resulting vector");
    set_default_filename("config/test/zero_sharing.ini");
  }
};

int main(int argc, const char *argv[]) {
  using T = uint64_t;
  // parse config
  zero_sharing_config conf;
  try {
    conf.parse(argc, argv);
  } catch (boost::program_options::error &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  // connect to other party
  party p(conf);
  auto channel = p.connect_to(1 - p.get_id());

  int seed = 12345;  // seed random number generator deterministically
  std::mt19937 prg(seed);
  std::uniform_int_distribution<T> dist;
  size_t k = conf.num_nonzeros, n = conf.len;
  std::vector<T> v(k), v2, result, result2;
  std::vector<size_t> I;
  if (p.get_id() == 0) {
    std::cout << "Generating random data\n";
    I = reservoir_sampling(prg, k, n);
    std::generate(v.begin(), v.end(), [&] { return dist(prg); });
    result = zero_sharing_server(v, I, n, channel);
    channel.send(I);
  } else {
    std::generate(v.begin(), v.end(), [&] { return dist(prg); });
    result = zero_sharing_client(v, n, channel);
    channel.recv(I);
  }
  channel.send_recv(v, v2);
  channel.send_recv(result, result2);
  std::vector<T> wanted(n, 0);
  for (size_t i = 0; i < k; i++) {
    wanted[I[i]] = v[i] + v2[i];
  }
  for (size_t i = 0; i < n; i++) {
    T got = result[i] + result2[i];
    if (wanted[i] != got) {
      std::cerr << "Error in element " << i << ": wanted " << wanted[i]
                << ", got " << got << "\n";
    }
  }
  return 0;
}
