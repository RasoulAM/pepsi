#include "client.h"
#include <iomanip>
#include <fstream>

using namespace std;
using namespace seal;

// Ideal for batch encoding
void Client::setup_crypto(uint64_t log_poly_modulus_degree, vector<Modulus> coeff_bits, uint64_t prime_bitlength, bool _verbose)
{

    /* Parameter Selection */
    this->enc_params = new EncryptionParameters(scheme_type::bfv);
    this->enc_params->set_poly_modulus_degree(1 << log_poly_modulus_degree);

    // Setting the coeffcient modulus
    // this->enc_params->set_coeff_modulus(CoeffModulus::BFVDefault(1 << log_poly_modulus_degree));
    this->enc_params->set_coeff_modulus(coeff_bits);
    // for (Modulus mod: this->enc_params->coeff_modulus()) {
    //     cout << mod.bit_count() << " ";
    // } cout << endl;

    this->enc_params->set_plain_modulus(PlainModulus::Batching(1 << log_poly_modulus_degree, prime_bitlength));

    this->context = new SEALContext(*(this->enc_params));
    this->keygen = new KeyGenerator(*(this->context));
    this->secret_key = keygen->secret_key();
    this->encryptor = new Encryptor(*(this->context), secret_key);
    this->batch_encoder = new BatchEncoder(*(this->context));

    if (_verbose) {
        uint64_t coeff_bitcount = 0;
        for (auto& prime : this->enc_params->coeff_modulus()) {
            coeff_bitcount += prime.bit_count();
        }
        cout << "-------------------------- Crypto Parameters "
                "--------------------------"
             << endl
             << "\tPoly Mod Degree: " << this->enc_params->poly_modulus_degree() << endl
             << "\tPlain Modulus: " << this->enc_params->plain_modulus().value() << " ("
             << this->enc_params->plain_modulus().bit_count() << " bits)" << endl
             << "\tCiphertext Modulus Bitcount: " << coeff_bitcount << endl;
    }

}

void Client::send_parameters_and_keys(bool debug_mode)
{

    this->metrics->metrics_["comm_params"] = this->enc_params->save(this->params_stream);

    // If require custom rotation keys, uncomment below
    // vector<uint32_t> elts;
    // for (int i=2; i<=this->enc_params->poly_modulus_degree(); i*=2) {
    //     elts.push_back(i+1);
    // }
    Serializable<RelinKeys> rlk_client = this->keygen->create_relin_keys();
    this->metrics->metrics_["comm_relin_keys"] = rlk_client.save(this->params_stream);
    Serializable<GaloisKeys> gal_keys_client = this->keygen->create_galois_keys();
    this->metrics->metrics_["comm_gal_keys"] = gal_keys_client.save(this->params_stream);
    Serializable<PublicKey> public_key = this->keygen->create_public_key();
    this->metrics->metrics_["comm_pk"] = public_key.save(this->params_stream);
    
    // If debugging
    if (debug_mode) this->secret_key.save(this->params_stream);
}

uint64_t Client::client_get(uint64_t gamma_prime, uint64_t bin_index, uint64_t effective_lambda){
    return bin_index*gamma_prime % (1 << effective_lambda);
}

void Client::encrypt_and_send(vector<Plaintext>& plaintexts, bool _verbose) {
    data_stream.str("");
    std::vector<Plaintext> result;
    this->metrics->metrics_["comm_request"] = 0;
    for (int i = 0; i < plaintexts.size(); i++) {
        this->metrics->metrics_["comm_request"] += this->encryptor->encrypt_symmetric(plaintexts[i]).save(data_stream);
    }
}

vector<Plaintext> Client::load_and_decrypt(int num_cts, bool _verbose) {
    std::vector<Plaintext> result;
    Decryptor decryptor(*context, this->secret_key);
    Ciphertext __temp_ct;
    Plaintext __temp_pt;
    this->metrics->metrics_["comm_response"] = 0;
    for (int i = 0; i < num_cts; i++) {
        this->metrics->metrics_["comm_response"] += __temp_ct.load(*(this->context), data_stream);
        decryptor.decrypt(__temp_ct, __temp_pt);
        if (i==0){
            cout << "Final Noise: " << decryptor.invariant_noise_budget(__temp_ct) << endl;
        }
        result.push_back(__temp_pt);
    }
    return result;
}

// Take as input a Params object and add all the fields to the metrics
void Client::params_to_metrics(Params params) {
    this->metrics->metrics_["gamma"] = params.gamma;
    this->metrics->metrics_["ell"] = params.ell;
    this->metrics->metrics_["hw"] = params.hw;
    this->metrics->metrics_["log_poly_modulus_degree"] = params.log_poly_modulus_degree;
    this->metrics->metrics_["effective_lambda"] = params.effective_lambda;
    this->metrics->metrics_["mu"] = params.mu;
    this->metrics->metrics_["task"] = params.task;
    this->metrics->metrics_["bitlength"] = params.bitlength;
}

// End to End
bool Client::run_protocol(Params params, bool _verbose)
{

    Timer time_setup_crypto, time_server_total, time_server_crypto, time_server_latency, time_extract_response;
    this->metrics = new Metrics();
    this->params_to_metrics(params);

    if (_verbose)
        cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
                "<<<<<<<<<<<<<<<<<<<<<<"
             << endl;

    bool debug_mode=false;

    uint64_t hw = params.hw;

    if (_verbose) {
        cout << "-------------------------- PSI Parameters "
                "--------------------------"
             << endl
             << "\tHamming Weight: " << params.hw << endl
             << "\tEffective Bitlength: " << params.effective_lambda << endl
             << "\tCode Length: " << params.ell << endl;
    }

    time_setup_crypto.start();
        this->setup_crypto(params.log_poly_modulus_degree, params.coeff_bits, params.prime_bitlength, _verbose);
    this->metrics->metrics_["time_setup_crypto"] = time_setup_crypto.end_and_get();

    // Sending Parameters
    this->send_parameters_and_keys(debug_mode);
    this->server->load_parameters_and_keys(this->params_stream, debug_mode, _verbose);

    vector<Plaintext> input;

        Plaintext operand_pt;
        vector<vector<uint64_t>> operands_vec(params.ell,vector<uint64_t>(1<<params.log_poly_modulus_degree, 0));        
        
        for (uint64_t gamma_prime=0; gamma_prime<params.gamma; gamma_prime++){
            for (int bin_index=0;bin_index<1<<params.log_poly_modulus_degree;bin_index++){
                uint64_t element = client_get(gamma_prime, bin_index, params.effective_lambda);
                vector<uint64_t> code = get_cw(element, params.ell, params.hw, true);
                for (int l=0;l<params.ell;l++){
                    operands_vec[l][bin_index] = code[l];
                }
            }
            for (int l=0;l<params.ell;l++){
                this->batch_encoder->encode(operands_vec[l], operand_pt);
                input.push_back(operand_pt);
            }
        }

    this->encrypt_and_send(input, _verbose);

    // Invoking server
    time_server_latency.start();
        this->server->do_server_computation(data_stream, input.size(), params, this->metrics, _verbose);
    metrics->metrics_["time_server_latency"] = time_server_latency.end_and_get();

    // Extracting Response
    time_extract_response.start();
        int num_cts; // Specify how many cts are sent over the network
        switch (params.task){
            case PSI:
                num_cts = params.gamma;
                break;
            case LabelledPSI:
                num_cts = params.gamma * params.label_ct_size;
                break;
            case PSICardinality:
                num_cts = 1;
                break;
            case PSISum:
                num_cts = 1;
                break;
        }

        std::vector<Plaintext> _response_pts = this->load_and_decrypt(num_cts, _verbose);
        // Interpret the result here
        // cout << "Example Response: " << _response_pts[0].to_string() << endl;
    this->metrics->metrics_["time_extract_response"] = time_extract_response.end_and_get();

    // Printing timings
    if (_verbose) {
        cout << "------------------------ Timing "
                "----------------------------------------------"
             << endl;
        cout << "\tTotal Server    : " << setw(10)
             << this->metrics->metrics_["time_server"] << " ms" << endl;
             
        cout << "--------------------- Communication "
                "------------------------------------------"
             << endl << "\tData Independant: "
             << this->metrics->metrics_["comm_relin_keys"] / (float)1000000 << " MB (Relin keys) + "
             << this->metrics->metrics_["comm_gal_keys"] / 1000000 << " MB (Gal Keys) + " 
             << this->metrics->metrics_["comm_pk"] / 1000 << " KB (Public Keys)" 
             << endl << "\tData Dependant: "
             << this->metrics->metrics_["comm_request"] / 1000 << " KB (Query) + "
             << this->metrics->metrics_["comm_response"] / 1000 << " KB (Reponse)" << endl
             << endl
             << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
                "<<<<<<<<<<<<<<<<<<<<<<"
             << endl;
    }

    bool write_to_file=true;
    if (write_to_file) {
        uint64_t filename = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() % 100000000000;

        string filename_string="results/" + to_string(filename) + ".csv";
        cout << "Writing to file " << filename_string << endl;

        // rely on the user to ensure that results directory exists
        ofstream outFile;
        outFile.open(filename_string);
        for (pair<string, uint64_t> metric : this->metrics->metrics_) {
            outFile << metric.first << "," << metric.second << endl;
        }
    }
    return true;
}