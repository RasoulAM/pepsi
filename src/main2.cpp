#include <iostream>
#include <getopt.h>
#include "utils.h"
#include "client.h"
#include "server.h"

using namespace std;


/*
    This code takes in the effective bitlength and hamming weight.
    mu and gamma and b_0 is fixed
    This function can be used to optimize protocol.
    This can be done by iterating over different Hamming weights and observing the comm. and comp.
*/

// main function with arguements
int main(int argc, char* argv[]) {
    cout << "Hello World!" << endl; 
    
    // The number of threads is set to 1 to be able to optimize the communication cost without noise
    omp_set_num_threads(1);

    // These don't matter for this experiement
    uint64_t mu = 100;
    uint64_t gamma = 1;
    Task task = PSI;

    // These two will vary
    uint64_t effective_bitlength = 19;
    uint64_t hw=8;

    static struct option long_options[] = {
        {"effective_bitlength", optional_argument, 0, 'l'},
        {"hamming_weight", optional_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "m:g:t:l:h:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'l':
                effective_bitlength = strtoull(optarg, nullptr, 10);
                break;
            case 'h':
                hw = strtoull(optarg, nullptr, 10);
                break;
            case '?':
                // getopt_long will print an error message
                return 1;
            default:
                std::abort();
        }
    }

    Server* server = new Server();
    Client client(server);

    Params params(task, mu, gamma, effective_bitlength, hw, 1);
    client.run_protocol(params);

}