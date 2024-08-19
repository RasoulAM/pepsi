#include <iostream>
#include <getopt.h>

#include "utils.h"
#include "client.h"
#include "server.h"



using namespace std;

// main function with arguements
int main(int argc, char* argv[]) {
    cout << "Hello World!" << endl; 
    omp_set_num_threads(32);

    // Timer timer;

    // timer.start();

    // #pragma omp parallel for
    // for (int i=0;i<(1<<20);i++){
    //     if (i%(1<<24)==0){
    //         cout << i << endl;
    //     }
    //     string str = "Meet";
    //     size_t str_hash = hash<string>{}(str);
    // }

    // cout << timer.end_and_get() << endl;

    // return 0;

    // Creat variables for mu, gamma, task, and effective_bitlength
    uint64_t mu = 250;
    uint64_t gamma = 1;
    uint64_t effective_bitlength = 19;
    Task task = LabelledPSI;
    uint64_t type = 1;

    static struct option long_options[] = {
        {"mu", optional_argument, 0, 'm'},
        {"gamma", optional_argument, 0, 'g'},
        {"task", optional_argument, 0, 't'},
        {"effective_bitlength", optional_argument, 0, 'l'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "m:g:t:l:x:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'm':
                mu = strtoull(optarg, nullptr, 10);
                break;
            case 'g':
                gamma = strtoull(optarg, nullptr, 10);
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
                effective_bitlength = strtoull(optarg, nullptr, 10);
                break;
            case 'x':
                type = strtoull(optarg, nullptr, 10);
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

    vector<uint64_t> best_comm_hw({
        0,0,0,0,
        2, 2, 2, 4, 4, 4, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 12, 13, 13, 14, 15, 15, 16, 16, 15, 15, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 23, 23,
        24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30
    });

    // TODO: Should be updated
    vector<uint64_t> best_comp_hw({
        0,0,0,0,
        1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4,
        4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 6,
        6, 6, 6, 7, 6, 7, 7, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8
    });    

    uint64_t label_byte_size = 1;

    uint64_t hw=8;
    if (type == 1){
        hw=best_comm_hw[effective_bitlength];
    } else if (type == 2){
        hw=best_comp_hw[effective_bitlength];
    }
    Params params(task, mu, gamma, effective_bitlength, hw, label_byte_size);
    client.run_protocol(params);

}