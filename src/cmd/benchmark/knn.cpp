#include "src/util/knn.hpp"

// generates random matrices and multiplies them using multiplication triples
int main(int argc, const char *argv[]) {
  using namespace sparse_linear_algebra::util::knn;
  int precision = 10;

  KNNConfig conf;
  try {
    conf.parse(argc, argv);
  } catch (boost::program_options::error &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  // connect to other party
  party p(conf);
  auto channel = p.connect_to(1 - p.get_id());
  try {
    runExperiments(&channel, p.get_id(), conf.statistical_security, precision, conf);
  } catch (boost::exception &ex) {
    std::cerr << boost::diagnostic_information(ex);
    return 1;
  }
}
