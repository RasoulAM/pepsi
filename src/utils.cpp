#include "utils.h"

using namespace std;
using namespace seal;

Timer::Timer(){
    start_ = chrono::steady_clock::now();
    end_ = chrono::steady_clock::now();
}

void Timer::start(){
    start_ = chrono::steady_clock::now();
}

void Timer::end(){
    end_ = chrono::steady_clock::now();
}

long double Timer::end_and_get(){
    end_ = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end_ - start_);
    return elapsed.count();
}

long double Timer::get_time_in_milliseconds(){
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end_ - start_);
    return elapsed.count();
}

float log2_choose(uint64_t n, uint64_t k) {
    if (k > n) {
        return 0;
    }
    float r = 0.0;
    for (uint64_t d = 1; d <= k; ++d) {
        r += log2(n--)-log2(d);
    }
    return r;
}

uint64_t choose(uint64_t n, uint64_t k) {
    if (k > n) {
        return 0;
    }
    uint64_t r = 1;
    for (uint64_t d = 1; d <= k; ++d) {
        r *= n--;
        r /= d;
    }
    return r;
}

vector<uint64_t> get_cw(uint64_t __number, uint64_t encoding_size, uint64_t hamming_weight, bool __verbose){
    vector<uint64_t> ans(encoding_size, 0ULL);
    uint64_t mod_size = choose(encoding_size, hamming_weight);
    if (__number >= mod_size){
        if (__verbose){
            cout << "Overflow occurred, everything okay?" << endl;
            cout << __number << " " << mod_size << endl;
        }
        __number %= mod_size;
    }
    long remainder = __number, k_prime = hamming_weight;
    for (long pointer=encoding_size-1; pointer>=0; pointer--){
        if (remainder >= choose(pointer, k_prime)){
            ans[pointer] = 1ULL;
            remainder -= choose(pointer, k_prime);
            k_prime -= 1;
        }
    }
    return ans;
}

Params::Params(Task task, uint64_t mu, uint64_t gamma, uint64_t effective_lambda, uint64_t hw, uint64_t label_byte_size){

    this->task = task;
    this->mu = mu;
    this->gamma = gamma;
    this->effective_lambda = effective_lambda;
    this->hw = hw;

    this->ell = hw;
    while (log2_choose(this->ell, this->hw) < this->effective_lambda){
        this->ell++;
    }

    // Valid Parameters
    // N=12 -> h <=  2 -> 8
    
    // N=13 -> h <=  4 -> 77   | 
    // N=13 -> h <=  8 -> 45   | 
    // N=13 -> h <= 16 -> 12   | 218
    
    // N=14 -> h <= 32 -> 191
    // N=14 -> h <= 64 -> 158    

    if (hw <= 1){ // q=72
        log_poly_modulus_degree = 12;
        // coeff_bits = CoeffModulus::BFVDefault(1 << log_poly_modulus_degree);
        coeff_bits = CoeffModulus::Create(1 << log_poly_modulus_degree, {36,36});
    } else if(hw <= 2){ // q=48*3=144
        log_poly_modulus_degree = 13;
        // coeff_bits = CoeffModulus::BFVDefault(1 << log_poly_modulus_degree);
        coeff_bits = CoeffModulus::Create(1 << log_poly_modulus_degree, {48,48,48});
    } else if(hw <= 4){ // q=48*2+36*2=168
        log_poly_modulus_degree = 13;
        coeff_bits = CoeffModulus::Create(1 << log_poly_modulus_degree, {48,36,36,48});
    } else if(hw <= 8){ // q=48*2+36*3=204
        log_poly_modulus_degree = 13;
        // coeff_bits = CoeffModulus::BFVDefault(1 << log_poly_modulus_degree);
        coeff_bits = CoeffModulus::Create(1 << log_poly_modulus_degree, {48,36,36,36,48});
    } else if(hw <= 16){ // q=48*2+36*4=240
        log_poly_modulus_degree = 14;
        // coeff_bits = CoeffModulus::BFVDefault(1 << log_poly_modulus_degree);
        coeff_bits = CoeffModulus::Create(1 << log_poly_modulus_degree, {48,36,36,36,36,48});
    } else if(hw <= 32){ // q=48*2+36*5=276
        log_poly_modulus_degree = 14;
        // coeff_bits = CoeffModulus::BFVDefault(1 << log_poly_modulus_degree);
        coeff_bits = CoeffModulus::Create(1 << log_poly_modulus_degree, {48,36,36,36,36,36,48});
    } else if(hw <= 64){ // q=48*2+36*6=312
        log_poly_modulus_degree = 14;
        // coeff_bits = CoeffModulus::BFVDefault(1 << log_poly_modulus_degree);
        coeff_bits = CoeffModulus::Create(1 << log_poly_modulus_degree, {48,36,36,36,36,36,36,48});
    }

    this->bitlength = this->effective_lambda + this->log_poly_modulus_degree;
    
    this->label_byte_size=label_byte_size;

    this->prime_bitlength=20;
    this->label_ct_size = ceil((float)this->label_byte_size * 8 / this->prime_bitlength);
};