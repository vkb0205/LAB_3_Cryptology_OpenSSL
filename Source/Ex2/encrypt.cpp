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
        std::cerr << "Usage: " << argv[0] << " <pub.pem> <plaintext_file> <output_cipher>\n";
        return 1;
    }

    const char* pub_path = argv[1];
    const char* plaintext_path = argv[2];
    const char* cipher_path = argv[3];

    // Load public key (PEM)
    FILE* key_fp = fopen(pub_path, "r");
    if (!key_fp) {
        std::cerr << "Cannot open public key file: " << pub_path << std::endl;
        return 1;
    }
    EVP_PKEY* pubKey = PEM_read_PUBKEY(key_fp, NULL, NULL, NULL);
    fclose(key_fp);
    if (!pubKey) handleErrors();

    // Prepare encryption context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pubKey, NULL);
    if (!ctx) handleErrors();
    if (EVP_PKEY_encrypt_init(ctx) <= 0) handleErrors();
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) handleErrors();

    // Read plaintext
    std::ifstream inFile(plaintext_path, std::ios::binary);
    if (!inFile) {
        std::cerr << "Cannot open plaintext file: " << plaintext_path << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pubKey);
        return 1;
    }
    std::vector<unsigned char> plain((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

    // Encrypt
    size_t outlen = 0;
    if (EVP_PKEY_encrypt(ctx, NULL, &outlen, plain.data(), plain.size()) <= 0) {
        std::cerr << "Error: Encryption failed (Is the file too big for RSA?)\n";
        handleErrors();
    }

    std::vector<unsigned char> cipher(outlen);
    if (EVP_PKEY_encrypt(ctx, cipher.data(), &outlen, plain.data(), plain.size()) <= 0) handleErrors();

    // Write ciphertext
    std::ofstream outFile(cipher_path, std::ios::binary);
    if (!outFile) {
        std::cerr << "Cannot write output file: " << cipher_path << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pubKey);
        return 1;
    }
    outFile.write(reinterpret_cast<const char*>(cipher.data()), outlen);

    std::cout << "Encryption successful! Output: " << cipher_path << std::endl;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pubKey);
    return 0;
}