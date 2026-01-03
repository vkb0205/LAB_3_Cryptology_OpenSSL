#include <iostream>
#include <fstream>
#include <vector>
#include <openssl/evp.h>
#include <openssl/pem.h>
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

    // 1. Load Private Key
    FILE* key_fp = fopen(argv[1], "r");
    if (!key_fp) handleErrors();
    EVP_PKEY* privKey = PEM_read_PrivateKey(key_fp, NULL, NULL, NULL);
    fclose(key_fp);
    if (!privKey) handleErrors();

    // 2. Create Signing Context (Using SHA-256)
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) handleErrors();

    // Initialize signing with SHA-256
    if (EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, privKey) <= 0) handleErrors();

    // 3. Read Message & Feed to Signer
    std::ifstream file(argv[2], std::ios::binary);
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
    std::ofstream outfile(argv[3], std::ios::binary);
    outfile.write(reinterpret_cast<const char*>(signature.data()), sig_len);

    std::cout << "Signature created successfully: " << argv[3] << std::endl;

    // Cleanup
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(privKey);
    return 0;
}