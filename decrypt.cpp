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
        std::cerr << "Usage: " << argv[0] << " <priv.pem> <ciphertext_file> <output_plain>\n";
        return 1;
    }

    // 1. Load Private Key
    FILE* key_fp = fopen(argv[1], "r");
    if (!key_fp) handleErrors();
    EVP_PKEY* privKey = PEM_read_PrivateKey(key_fp, NULL, NULL, NULL);
    fclose(key_fp);
    if (!privKey) handleErrors();

    // 2. Setup Context for Decryption
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privKey, NULL);
    if (!ctx) handleErrors();
    if (EVP_PKEY_decrypt_init(ctx) <= 0) handleErrors();
    
    // Explicitly set PKCS#1 padding to match encryption
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) handleErrors();

    // 3. Read Ciphertext
    std::ifstream inFile(argv[2], std::ios::binary);
    std::vector<unsigned char> cipher((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

    // 4. Decrypt
    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, NULL, &outlen, cipher.data(), cipher.size()) <= 0) handleErrors();

    std::vector<unsigned char> plain(outlen);
    if (EVP_PKEY_decrypt(ctx, plain.data(), &outlen, cipher.data(), cipher.size()) <= 0) handleErrors();

    // 5. Write Plaintext
    std::ofstream outFile(argv[3], std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(plain.data()), outlen);

    std::cout << "Decryption successful! Output: " << argv[3] << std::endl;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(privKey);
    return 0;
}