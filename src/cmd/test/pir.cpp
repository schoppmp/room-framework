#include "fastpoly/recursive.h"
#include "mpc-utils/mpc_config.hpp"
#include <NTL/vector.h>

class test_pir_config : public virtual mpc_config {
protected:
  void validate() {
    namespace po = boost::program_options;
    if(num_elements_client <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'num_elements_client' must be positive"));
    }
    if(num_elements_server <= 0) {
      BOOST_THROW_EXCEPTION(po::error("'num_elements_server' must be positive"));
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
  // initialize 128-bit prime field (order FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF61)
  NTL::ZZ_p::init(
    NTL::conv<NTL::ZZ>("340282366920938463463374607431768211297")
  );

  // generate server values (just take the first N primes)
  NTL::Vec<NTL::ZZ> elements_server;
  elements_server.SetLength(conf.num_elements_server);
  NTL::PrimeSeq primes;
  std::generate(
    elements_server.begin(),
    elements_server.end(),
    [&]() { return primes.next(); }
  );
  std::cout << 10 * elements_server;

}
