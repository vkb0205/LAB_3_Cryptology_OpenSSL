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
        std::cerr << "Usage: " << argv[0] << " <priv.pem> <message_file> <output_signature>\n";
        return 1;
    }

    const char* priv_path = argv[1];
    const char* msg_path = argv[2];
    const char* sig_path = argv[3];

    // 1. Load Private Key
    FILE* key_fp = fopen(priv_path, "r");
    if (!key_fp) {
        std::cerr << "Cannot open private key file: " << priv_path << std::endl;
        return 1;
    }
    EVP_PKEY* privKey = PEM_read_PrivateKey(key_fp, NULL, NULL, NULL);
    fclose(key_fp);
    if (!privKey) handleErrors();

    // 2. Create Signing Context (Using SHA-256)
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) handleErrors();

    // Initialize signing with SHA-256
    if (EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, privKey) <= 0) handleErrors();

    // 3. Read Message & Feed to Signer
    std::ifstream file(msg_path, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open message file: " << msg_path << std::endl;
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(privKey);
        return 1;
    }
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    if (EVP_DigestSignUpdate(mdctx, buffer.data(), buffer.size()) <= 0) handleErrors();

    // 4. Finalize Signature
    size_t sig_len = 0;
    // First call determines buffer size
    if (EVP_DigestSignFinal(mdctx, NULL, &sig_len) <= 0) handleErrors();
    
    std::vector<unsigned char> signature(sig_len);
    // Second call actually signs
    if (EVP_DigestSignFinal(mdctx, signature.data(), &sig_len) <= 0) handleErrors();

    // 5. Write Signature to File
    std::ofstream outfile(sig_path, std::ios::binary);
    if (!outfile) {
        std::cerr << "Cannot write signature file: " << sig_path << std::endl;
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(privKey);
        return 1;
    }
    outfile.write(reinterpret_cast<const char*>(signature.data()), sig_len);

    std::cout << "Signature created successfully: " << sig_path << std::endl;

    // Cleanup
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(privKey);
    return 0;
}