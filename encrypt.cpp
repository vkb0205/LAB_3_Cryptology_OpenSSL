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
        std::cerr << "Usage: " << argv[0] << " <pub.pem> <plaintext_file> <output_cipher>\n";
        return 1;
    }

    // 1. Load Public Key
    FILE* key_fp = fopen(argv[1], "r");
    if (!key_fp) handleErrors();
    EVP_PKEY* pubKey = PEM_read_PUBKEY(key_fp, NULL, NULL, NULL);
    fclose(key_fp);
    if (!pubKey) handleErrors();

    // 2. Setup Context for Encryption
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pubKey, NULL);
    if (!ctx) handleErrors();
    if (EVP_PKEY_encrypt_init(ctx) <= 0) handleErrors();
    
    // Explicitly set PKCS#1 padding (Standard for OpenSSL pkeyutl)
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) handleErrors();

    // 3. Read Plaintext
    std::ifstream inFile(argv[2], std::ios::binary);
    std::vector<unsigned char> plain((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

    // 4. Encrypt
    size_t outlen;
    // Determine required buffer size
    if (EVP_PKEY_encrypt(ctx, NULL, &outlen, plain.data(), plain.size()) <= 0) {
        std::cerr << "Error: Encryption failed (Is the file too big for RSA?)\n";
        handleErrors();
    }

    std::vector<unsigned char> cipher(outlen);
    if (EVP_PKEY_encrypt(ctx, cipher.data(), &outlen, plain.data(), plain.size()) <= 0) handleErrors();

    // 5. Write Ciphertext
    std::ofstream outFile(argv[3], std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(cipher.data()), outlen);

    std::cout << "Encryption successful! Output: " << argv[3] << std::endl;

    // Cleanup
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pubKey);
    return 0;
}