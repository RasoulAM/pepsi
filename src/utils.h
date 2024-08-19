#ifndef __UTILS__H
#define __UTILS__H

#include "seal/seal.h"
#include "omp.h"

using namespace std;

class Timer {
public:
    std::chrono::steady_clock::time_point start_;
    std::chrono::steady_clock::time_point end_;
    Timer();
    void start();
    void end();
    long double end_and_get();
    void reset();
    long double get_time_in_milliseconds();
};

class Metrics{
    public:
    map<string, uint64_t> metrics_;
};

enum Task{
    PSI=0,
    LabelledPSI=1,
    PSICardinality=2,
    PSISum=3
};

// return n choose k
float log2_choose(uint64_t n, uint64_t k);
vector<uint64_t> get_cw(uint64_t __number, uint64_t encoding_size, uint64_t hamming_weight, bool __verbose);

class Params{

    public:

    uint64_t log_poly_modulus_degree;

    Task task;

    uint64_t mu;
    uint64_t gamma;
    uint64_t effective_lambda;
    uint64_t hw;

    uint64_t ell;
    uint64_t bitlength;

    uint64_t prime_bitlength;
    uint64_t label_byte_size;
    uint64_t label_ct_size;

    vector<seal::Modulus> coeff_bits;

    Params();
    Params(Task task, uint64_t mu, uint64_t gamma, uint64_t effective_lambda, uint64_t hw, uint64_t label_byte_size);

};

#endif
