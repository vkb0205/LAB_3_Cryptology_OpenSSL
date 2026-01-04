#include <iostream>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/bn.h>
#include <openssl/err.h>

// Print a BIGNUM on one line with its bit-length.
void print_bn_line(const char* label, const BIGNUM* bn) {
    if (bn == nullptr) {
        std::cout << "- " << label << ": (missing)" << std::endl;
        return;
    }
    char* dec_str = BN_bn2dec(bn);
    std::cout << "- " << label << ": " << dec_str;
    std::cout << std::endl;
    OPENSSL_free(dec_str);
}

int main() {
    const char* private_key_file = "priv.pem";
    const char* public_key_file = "pub.pem";

    // Read private key
    FILE* fp_private = fopen(private_key_file, "r");
    if (!fp_private) {
        std::cerr << "Cannot open file priv.pem" << std::endl;
        return 1;
    }
    EVP_PKEY* prikey = PEM_read_PrivateKey(fp_private, NULL, NULL, NULL);
    fclose(fp_private);

    // Read public key
    FILE* fp_public = fopen(public_key_file, "r");
    if (!fp_public) {
        EVP_PKEY_free(prikey);
        std::cerr << "Cannot open file pub.pem" << std::endl;
        return 1;
    }
    EVP_PKEY* pubkey = PEM_read_PUBKEY(fp_public, NULL, NULL, NULL);
    fclose(fp_public);

    if (!prikey || !pubkey) {
        EVP_PKEY_free(prikey);
        EVP_PKEY_free(pubkey);
        std::cerr << "Error reading PEM file." << std::endl;
        return 1;
    }

    // Get RSA parameters directly from EVP_PKEY (OpenSSL 3 safe)
    BIGNUM *n = nullptr, *e = nullptr, *d = nullptr;
    BIGNUM *p = nullptr, *q = nullptr;
    BIGNUM *exp1 = nullptr, *exp2 = nullptr, *coeff = nullptr;
    BIGNUM *pub_n = nullptr, *pub_e = nullptr;

    auto fetch_param = [&](EVP_PKEY* key, const char* name, BIGNUM*& out) {
        if (!EVP_PKEY_get_bn_param(key, name, &out)) {
            std::cerr << "Cannot get parameter " << name << std::endl;
        }
    };

    // Private key params
    fetch_param(prikey, OSSL_PKEY_PARAM_RSA_N, n);
    fetch_param(prikey, OSSL_PKEY_PARAM_RSA_E, e);
    fetch_param(prikey, OSSL_PKEY_PARAM_RSA_D, d);
    fetch_param(prikey, OSSL_PKEY_PARAM_RSA_FACTOR1, p);
    fetch_param(prikey, OSSL_PKEY_PARAM_RSA_FACTOR2, q);
    fetch_param(prikey, OSSL_PKEY_PARAM_RSA_EXPONENT1, exp1);
    fetch_param(prikey, OSSL_PKEY_PARAM_RSA_EXPONENT2, exp2);
    fetch_param(prikey, OSSL_PKEY_PARAM_RSA_COEFFICIENT1, coeff);

    // Public key params
    fetch_param(pubkey, OSSL_PKEY_PARAM_RSA_N, pub_n);
    fetch_param(pubkey, OSSL_PKEY_PARAM_RSA_E, pub_e);

    int bits = n ? BN_num_bits(n) : (pub_n ? BN_num_bits(pub_n) : 0);

    std::cout << "Private Key Information:" << std::endl;
    std::cout << "RSA Private Key: (" << bits << " bit, 2 primes)" << std::endl;
    print_bn_line("Modulus (n)", n);
    print_bn_line("Public Exponent (e)", e);
    print_bn_line("Private Exponent (d)", d);
    print_bn_line("Prime1 (p)", p);
    print_bn_line("Prime2 (q)", q);
    print_bn_line("Exponent1 (d mod (p-1))", exp1);
    print_bn_line("Exponent2 (d mod (q-1))", exp2);
    print_bn_line("Coefficient (inverse of q mod p)", coeff);

    std::cout << std::endl;
    std::cout << "Public Key Information:" << std::endl;
    std::cout << "Public Key: (" << bits << " bit)" << std::endl;
    print_bn_line("Public Exponent (e)", pub_e ? pub_e : e);
    print_bn_line("Modulus (n)", pub_n ? pub_n : n);

    // Cleanup
    BN_free(n);
    BN_free(e);
    BN_free(d);
    BN_free(p);
    BN_free(q);
    BN_free(exp1);
    BN_free(exp2);
    BN_free(coeff);
    BN_free(pub_n);
    BN_free(pub_e);
    EVP_PKEY_free(prikey);
    EVP_PKEY_free(pubkey);
    return 0;
}