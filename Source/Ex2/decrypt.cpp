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
        std::cerr << "Usage: " << argv[0] << " <priv.pem> <ciphertext_file> <output_plain>\n";
        return 1;
    }

    const char* priv_path = argv[1];
    const char* cipher_path = argv[2];
    const char* plain_path = argv[3];

    // Load private key (PEM)
    FILE* key_fp = fopen(priv_path, "r");
    if (!key_fp) {
        std::cerr << "Cannot open private key file: " << priv_path << std::endl;
        return 1;
    }
    EVP_PKEY* privKey = PEM_read_PrivateKey(key_fp, NULL, NULL, NULL);
    fclose(key_fp);
    if (!privKey) handleErrors();

    // Prepare decryption context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privKey, NULL);
    if (!ctx) handleErrors();
    if (EVP_PKEY_decrypt_init(ctx) <= 0) handleErrors();
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) handleErrors();

    // Read ciphertext
    std::ifstream inFile(cipher_path, std::ios::binary);
    if (!inFile) {
        std::cerr << "Cannot open ciphertext file: " << cipher_path << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privKey);
        return 1;
    }
    std::vector<unsigned char> cipher((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

    // Decrypt
    size_t outlen = 0;
    if (EVP_PKEY_decrypt(ctx, NULL, &outlen, cipher.data(), cipher.size()) <= 0) handleErrors();

    std::vector<unsigned char> plain(outlen);
    if (EVP_PKEY_decrypt(ctx, plain.data(), &outlen, cipher.data(), cipher.size()) <= 0) handleErrors();

    // Write plaintext
    std::ofstream outFile(plain_path, std::ios::binary);
    if (!outFile) {
        std::cerr << "Cannot write output file: " << plain_path << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privKey);
        return 1;
    }
    outFile.write(reinterpret_cast<const char*>(plain.data()), outlen);

    std::cout << "Decryption successful! Output: " << plain_path << std::endl;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(privKey);
    return 0;
}