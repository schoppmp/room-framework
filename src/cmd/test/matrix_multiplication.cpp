#include "matrix_multiplication/dense.hpp"
#include "mpc-utils/mpc_config.hpp"
#include "mpc-utils/party.hpp"
#include "util/randomize_matrix.hpp"
#include "util/reservoir_sampling.hpp"
#include "util/time.h"
#include <Eigen/Dense>
#include <Eigen/Sparse>

// generates random matrices and multiplies them using multiplication triples
int main(int argc, const char *argv[]) {
    using T = uint32_t;
    using matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
    // parse config
    mpc_config conf;
    conf.set_default_filename("config/test/matrix_multiplication.ini");
    try {
        conf.parse(argc, argv);
    } catch (boost::program_options::error &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    // check validity
    if(conf.servers.size() < 2) {
        std::cerr << "At least two servers are needed for this test\n";
        return 1;
    }
    if(conf.party_id != 0 && conf.party_id != 1) {
        std::cerr << "party must be 0 or 1\n";
        return 1;
    }

    std::cout << "Connecting\n";
    // connect to other party
    party p(conf);
    auto channel = p.connect_to(1 - p.get_id());

    // generate test data
    size_t l = 5000, m = 50000, n = 1;
    size_t chunk_size = 1000;
    Eigen::SparseMatrix<T, Eigen::ColMajor> A(l, m);
    Eigen::SparseMatrix<T, Eigen::RowMajor> B(m, n);
    int seed = 12345; // seed random number generator deterministically
    std::mt19937 prg(seed);
    std::uniform_int_distribution<T> dist;


    std::cout << "Generating random data\n";
    size_t k_A = 2000;
    size_t k_B = 1000;

    auto indices_A = reservoir_sampling(prg, k_A, m);
    std::vector<Eigen::Triplet<T>> triplets_A;
    for(size_t j = 0; j < indices_A.size(); j++) {
      for(size_t i = 0; i < A.rows(); i++) {
        triplets_A.push_back(Eigen::Triplet<T>(i, j, dist(prg)));
      }
    }
    A.setFromTriplets(triplets_A.begin(), triplets_A.end());
    auto indices_B =  reservoir_sampling(prg, k_B, m);
    std::vector<Eigen::Triplet<T>> triplets_B;
    for(size_t i = 0; i < indices_B.size(); i++) {
      for(size_t j = 0; j < B.cols(); j++) {
        triplets_B.push_back(Eigen::Triplet<T>(i, j, dist(prg)));
      }
    }
    B.setFromTriplets(triplets_B.begin(), triplets_B.end());

    // run dense multiplication
    std::cout << "Running dense matrix multiplication\n";

    try {
        fake_triple_provider<T> triples(chunk_size, m, n, p.get_id());
        channel.sync();
        benchmark([&]{
          triples.precompute(l / chunk_size);
        }, "Fake Triple Generation");

        matrix C;
        channel.sync();
        benchmark([&]{
          C = matrix_multiplication(matrix(A), matrix(B), channel, p.get_id(), triples, chunk_size);
        }, "Dense matrix multiplication");

        // exchange shares for checking result
        std::cout << "Verifying\n";
        matrix C2;
        channel.send_recv(C, C2);
        if(p.get_id() == 0) {
            channel.send_recv(A, B);
        } else {
            channel.send_recv(B, A);
        }

        // verify result
        C += C2;
        C2 = A * B;
        for(size_t i = 0; i < C.rows(); i++) {
            for(size_t j = 0; j < C.cols(); j++) {
                if(C(i, j) != C2(i, j)) {
                    std::cerr << "Verification failed at index (" << i << ", " << j
                              << "). Expected " << C2(i, j) << ", got " << C(i, j) <<"\n";
                }
            }
        }
        std::cout << "Verification finished\n";
    } catch(boost::exception &ex) {
        std::cerr << boost::diagnostic_information(ex);
        return 1;
    }
}
