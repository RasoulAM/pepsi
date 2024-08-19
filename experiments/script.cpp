#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>

using namespace std;

void run_max_load(){
    uint64_t RUNS = ((uint64_t)1 << 30);

    uint64_t bins[] = {8192, 8192, 8192, 16384, 16384, 16384, 16384};
    uint64_t balls[] = {3145728,50331648,805306368,786432,3145728,12582912,50331648};

    int pairs = 6;

    int answer[pairs];
    for (int i=0;i<pairs;i++){
        answer[i] = 0;
    }
    uint64_t runned = 0;
    uint64_t milestone = 1;
    #pragma omp parallel
    {
        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        #pragma omp for
        for (int run=0;run < RUNS;run++){
            if (runned == milestone){
                #pragma omp critical
                {
                    if (runned == milestone){
                        cout << "Reached the mile stone " << milestone << endl;
                        milestone *= 2;
                        ofstream result_file;
                        result_file.open("simulation.csv", ios::app);
                        for (int i=0;i<pairs;i++){
                            result_file << milestone << ","
                                        << bins[i] << "," 
                                        << balls[i] << "," 
                                        << answer[i] << endl;
                        }
                        result_file.close();
                    }
                }
            }
            for (int i=0;i<pairs;i++){
                uint64_t NUM_BINS=bins[i];
                uint64_t NUM_BALLS=balls[i];

                std::uniform_int_distribution<> distrib(0, NUM_BINS-1);
                vector<int> bins(NUM_BINS,0);
                for (int j=0;j<NUM_BALLS;j++){
                    bins[distrib(gen)]++;
                }
                int this_max_load = *max_element(begin(bins), end(bins));
                #pragma omp critical
                {
                    if (this_max_load > answer[i])
                        answer[i] = this_max_load;
                }
            }
            #pragma omp critical
            {
                runned += 1;
            }
        }
    }
}

int main(){
    run_max_load();
}