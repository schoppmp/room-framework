#include "matrix_multiplication.hpp"
#include "mpc-utils/mpc_config.hpp"
#include "mpc-utils/party.hpp"
#include "util/randomize_matrix.hpp"
#include "util/reservoir_sampling.hpp"
#include "util/time.h"
#include <boost/numeric/ublas/matrix_sparse.hpp>

// generates random matrices and multiplies them using multiplication triples
int main(int argc, const char *argv[]) {
    using namespace boost::numeric::ublas;
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
    matrix<uint32_t, column_major> A(l, m);
    matrix<uint32_t, row_major> B(m, n);
    int seed = 12345; // seed random number generator deterministically
    std::mt19937 prg(seed);
    std::uniform_int_distribution<uint32_t> dist;


    std::cout << "Generating random data\n";
    size_t k_A = 2000;
    size_t k_B = 1000;

    A = zero_matrix<uint32_t>(l, m);
    matrix<uint32_t> nonzeros_A(l, k_A);
    randomize_matrix(prg, nonzeros_A);
    auto indices_A = reservoir_sampling(prg, k_A, m);
    for(size_t i = 0; i < indices_A.size(); i++) {
      column(A, indices_A[i]) = column(nonzeros_A, i);
    }
    B = zero_matrix<uint32_t>(m, n);
    matrix<uint32_t> nonzeros_B(k_B, n);
    randomize_matrix(prg, nonzeros_B);
    auto indices_B =  reservoir_sampling(prg, k_B, m);
    for(size_t i = 0; i < indices_B.size(); i++) {
      row(B, indices_B[i]) = row(nonzeros_B, i);
    }

    // run dense multiplication
    std::cout << "Running dense matrix multiplication\n";

    try {
        fake_triple_provider<uint32_t> triples(chunk_size, m, n, p.get_id());
        benchmark([&]{
          triples.precompute(l / chunk_size);
        }, "Fake Triple Generation");

        matrix<uint32_t> C;
        benchmark([&]{
          C = matrix_multiplication(A, B, channel, p.get_id(), triples, chunk_size);
        }, "Dense matrix multiplication");

        // exchange shares for checking result
        std::cout << "Verifying\n";
        matrix<uint32_t> C2;
        channel.send_recv(C, C2);
        if(p.get_id() == 0) {
            channel.send_recv(A, B);
        } else {
            channel.send_recv(B, A);
        }

        // verify result
        C += C2;
        C2 = prod(A, B);
        for(size_t i = 0; i < C.size1(); i++) {
            for(size_t j = 0; j < C.size2(); j++) {
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
