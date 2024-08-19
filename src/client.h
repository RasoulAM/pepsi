#ifndef __CLIENT__H
#define __CLIENT__H

#include "seal/seal.h"
#include "utils.h"
#include "server.h"

#include <vector>

class Client {
public:

    seal::EncryptionParameters* enc_params;

    seal::SEALContext* context;
    seal::KeyGenerator* keygen;
    seal::SecretKey secret_key;
    seal::Evaluator* evaluator;
    seal::Encryptor* encryptor;
    seal::BatchEncoder* batch_encoder;
    
    // Stringstreams
    std::stringstream params_stream;
    std::stringstream data_stream;

    Server* server;
    Metrics* metrics;

    // Preparation steps

    Client(){
        server = new Server();
    }
    
    Client(Server* _server){
        server = _server;
    }

    void params_to_metrics(Params params);
    void set_server(Server* server);
    uint64_t client_get(uint64_t gamma_prime, uint64_t bin_index, uint64_t effective_lambda);
    void setup_crypto(uint64_t log_poly_modulus_degree, vector<seal::Modulus> coeff_bits, uint64_t prime_bitlength=20, bool _verbose = true);
    void send_parameters_and_keys(bool debug_mode=false);
    void encrypt_and_send(vector<seal::Plaintext>& plaintexts, bool _verbose);
    bool run_protocol(Params params, bool _verbose = true);
    vector<seal::Plaintext> load_and_decrypt(int num_cts, bool _verbose = false);
};

#endif
