#include <iostream>
#include <fstream>
#include <vector>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

void handleErrors() {
    ERR_print_errors_fp(stderr);
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <pub.pem> <message_file> <signature_file>\n";
        return 1;
    }

    const char* pub_path = argv[1];
    const char* msg_path = argv[2];
    const char* sig_path = argv[3];

    // 1. Load Public Key
    FILE* key_fp = fopen(pub_path, "r");
    if (!key_fp) {
        std::cerr << "Cannot open public key file: " << pub_path << std::endl;
        return 1;
    }
    EVP_PKEY* pubKey = PEM_read_PUBKEY(key_fp, NULL, NULL, NULL);
    fclose(key_fp);
    if (!pubKey) handleErrors();

    // 2. Create Verification Context
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pubKey) <= 0) handleErrors();

    // 3. Read Message & Update Context
    std::ifstream msgFile(msg_path, std::ios::binary);
    if (!msgFile) {
        std::cerr << "Cannot open message file: " << msg_path << std::endl;
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pubKey);
        return 1;
    }
    std::vector<char> msgBuffer((std::istreambuf_iterator<char>(msgFile)), std::istreambuf_iterator<char>());
    if (EVP_DigestVerifyUpdate(mdctx, msgBuffer.data(), msgBuffer.size()) <= 0) handleErrors();

    // 4. Read Signature
    std::ifstream sigFile(sig_path, std::ios::binary);
    if (!sigFile) {
        std::cerr << "Cannot open signature file: " << sig_path << std::endl;
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pubKey);
        return 1;
    }
    std::vector<unsigned char> sigBuffer((std::istreambuf_iterator<char>(sigFile)), std::istreambuf_iterator<char>());

    // 5. Verify
    int result = EVP_DigestVerifyFinal(mdctx, sigBuffer.data(), sigBuffer.size());

    if (result == 1) {
        std::cout << "SUCCESS: Signature is Valid!" << std::endl;
    } else {
        std::cout << "FAILURE: Signature is Invalid!" << std::endl;
        ERR_print_errors_fp(stdout);
    }

    // Cleanup
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pubKey);
    return 0;
}