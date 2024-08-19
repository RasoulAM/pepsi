#include <iostream>
#include <getopt.h>
#include "utils.h"
#include "client.h"
#include "server.h"

using namespace std;

/*
    Perform PSI given only the client and server set size
    The bitlength is fixed to 32 bits
*/


// main function with arguements
int main(int argc, char* argv[]) {
    cout << "Hello World!" << endl; 
    omp_set_num_threads(32);

    // These three are fixed here
    uint64_t effective_bitlength = 19;
    uint64_t hw=8;

    uint64_t client_set=1024;
    uint64_t server_set=1<<20;
    Task task = PSI;
    uint64_t label_byte_size=1;

    static struct option long_options[] = {
        {"client_set", optional_argument, 0, 'c'},
        {"server_set", optional_argument, 0, 's'},
        {"task", optional_argument, 0, 't'},
        {"label", optional_argument, 0, 'l'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "c:s:t:l:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'c':
                client_set = strtoull(optarg, nullptr, 10);
                break;
            case 's':
                server_set = strtoull(optarg, nullptr, 10);
                break;
            case 't':
                if (std::string(optarg) == "PSI") {
                    task = PSI;
                } else if (std::string(optarg) == "LabelledPSI") {
                    task = LabelledPSI;
                } else if (std::string(optarg) == "PSICardinality") {
                    task = PSICardinality;
                } else if (std::string(optarg) == "PSISum") {
                    task = PSISum;
                } else {
                    std::cerr << "Unknown task: " << optarg << "\n";
                    return 1;
                }
                break;
            case 'l':
                label_byte_size = strtoull(optarg, nullptr, 10);
                break;
            case '?':
                // getopt_long will print an error message
                return 1;
            default:
                std::abort();
        }
    }

    uint64_t N=8192;
    
    uint64_t gamma = 1;
    uint64_t mu=1;
    uint64_t num_bins=1;
    uint64_t num_balls=1;

    gamma = ceil(client_set * 1.27 / N);
    
    num_bins = N*gamma;
    num_balls = 3 * server_set;
    
    mu = ceil((float)num_balls/num_bins + 2 * sqrt((float)num_balls*log2(num_bins)/num_bins));

    cout << "--- Specs ---" << endl;
    cout << "Client set size: " << client_set << endl;
    cout << "Server set size: " << server_set << endl;
    cout << "Task: " << task << endl;
    cout << "Label: " << label_byte_size << endl;

    Server* server = new Server();
    Client client(server);

    Params params(task, mu, gamma, effective_bitlength, hw, label_byte_size);
    client.run_protocol(params);

}
