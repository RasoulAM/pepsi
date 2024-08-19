#include "server.h"

#include <cassert>

using namespace std;
using namespace seal;

void Server::load_parameters_and_keys(stringstream& params_stream, bool debug_mode, bool _verbose)
{
    this->enc_params = new EncryptionParameters();
    this->enc_params->load(params_stream);

    this->context = new SEALContext(*enc_params);

    this->rlk_keys_server = new RelinKeys();
    this->gal_keys_server = new GaloisKeys();
    this->public_key = new PublicKey();
    this->rlk_keys_server->load(*context, params_stream);
    this->gal_keys_server->load(*context, params_stream);
    this->public_key->load(*context, params_stream);

    this->batch_encoder = new BatchEncoder(*context);
    this->encryptor = new Encryptor(*context, *public_key);
    this->evaluator = new Evaluator(*context);

    if (debug_mode){
        SecretKey secret_key;
        secret_key.load(*context, params_stream);
        this->noise_calculator = new Decryptor(*context, secret_key);
    }
}

vector<Ciphertext> Server::load_inputs(stringstream& data_stream, int num_input_cts){
    vector<Ciphertext> inputs;
    Ciphertext __ct_temp;
    for (int i=0;i<num_input_cts;i++){
        __ct_temp.load(*context, data_stream);
        inputs.push_back(__ct_temp);
    }
    data_stream.str("");
    return inputs;
}

void Server::send_results(vector<Ciphertext>& results, stringstream& data_stream){
    for (int i=0;i<results.size();i++){
        results[i].save(data_stream);
    }
}

uint64_t Server::server_get(uint64_t mu_prime, uint64_t bin_index, uint64_t effective_lambda){
    return bin_index*mu_prime % (1 << effective_lambda);
}


void Server::do_server_computation(stringstream& data_stream, int num_input_cts, Params params, Metrics* metrics, bool _verbose){
    Timer server_time;
    vector<Ciphertext> inputs = load_inputs(data_stream, num_input_cts);

    vector<Ciphertext> results;
    
    server_time.start();

        uint64_t bins = 1 << params.log_poly_modulus_degree;

        uint64_t mu = params.mu;
        uint64_t gamma = params.gamma;
        uint64_t ell = params.ell;
        uint64_t hw = params.hw;
        uint64_t effective_lambda = params.effective_lambda;
        uint64_t label_ct_size=params.label_ct_size;
        assert( gamma*ell == inputs.size() );

        vector<vector<Ciphertext>> indicators(gamma, vector<Ciphertext>(mu));
        vector<Ciphertext> indicators_sum(gamma);

        vector<Plaintext> addends_pt(hw-1);
        for (int i=0;i<hw-1;i++){
            this->batch_encoder->encode(vector<uint64_t>(this->batch_encoder->slot_count(), i+1), addends_pt[i]);
        }

        Plaintext labels_pt;
        this->batch_encoder->encode(vector<uint64_t>(this->batch_encoder->slot_count(), 3), labels_pt);


        Ciphertext zero_pt;
        encryptor->encrypt_zero(zero_pt);


        // Do some stuff here
        // Put the results in 'results'
        #pragma omp parallel for collapse(2)
        for (int i=0;i<mu;i++){
            for (int j=0;j<gamma;j++){

                Plaintext operand_pt;
                vector<vector<uint64_t>> operands_vec(ell,vector<uint64_t>(bins, 0));

                // get all the elements from (mu, gamma, 0) til (mu, gamma, b), get the perfect_constant_weight encoding for them
                for (int bin_index=0;bin_index<bins;bin_index++){
                    uint64_t element = server_get(i, bin_index, effective_lambda);
                    vector<uint64_t> code = get_cw(element, ell, hw, true);
                    for (int l=0;l<ell;l++){
                        operands_vec[l][bin_index] = code[l];
                    }
                }

                vector<Ciphertext> _temp(ell);
                for (int k=0;k<ell;k++){
                    if (accumulate(operands_vec[k].begin(), operands_vec[k].end(), 0) == 0){
                        _temp[k] = zero_pt;
                    } else {
                        this->batch_encoder->encode(operands_vec[k], operand_pt);
                        this->evaluator->multiply_plain(inputs[j*ell+k], operand_pt, _temp[k]);
                    }
                }

                // Add the subvector of ciphertexts from the same row
                Ciphertext _ct;
                this->evaluator->add_many(_temp, _ct);
                vector<Ciphertext> _operands;
                _operands.push_back(_ct);
                for (int k=0;k<hw-1;k++){
                    Ciphertext __ct;
                    this->evaluator->sub_plain(_ct, addends_pt[k], __ct);
                    _operands.push_back(__ct);
                }

                // Multiply many for operands
                this->evaluator->multiply_many(_operands, *rlk_keys_server, indicators[j][i]);
                for (int k=1;k<hw;k*=2){
                    this->evaluator->mod_switch_to_next_inplace(indicators[j][i]);
                }    

                // For PSI and PSICardinality, multiply by 1/k!
                // For LabelledPSI and PSISum, multiply by 1/k! * label
                this->evaluator->multiply_plain_inplace(indicators[j][i], labels_pt);
                Ciphertext __temp;
                for (int s=0;s<params.label_ct_size-1;s++){
                    this->evaluator->multiply_plain(indicators[j][i], labels_pt, __temp);
                }
            }
        }

        #pragma omp parallel for
        for (int i=0;i<gamma;i++){
            this->evaluator->add_many(indicators[i], indicators_sum[i]);
            this->evaluator->mod_switch_to_inplace(indicators_sum[i], this->context->last_context_data()->parms_id());
        }

        switch(params.task){
            case PSI:
                results = indicators_sum;
                break;
            case LabelledPSI:
                for (int s=0;s<params.label_ct_size;s++){
                    results.insert(results.end(), indicators_sum.begin(), indicators_sum.end());
                }
                break;
            case PSICardinality:
            case PSISum:
                Ciphertext result_temp;
                this->evaluator->add_many(indicators_sum, result_temp);
                results.push_back(result_temp);
                break;
        }

    server_time.end();
    metrics->metrics_["time_server"] = server_time.get_time_in_milliseconds();
    
    send_results(results, data_stream);
}