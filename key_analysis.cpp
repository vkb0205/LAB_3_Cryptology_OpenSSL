#include <iostream>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>  // Thư viện quan trọng để đọc Prime
#include <openssl/bn.h>
#include <openssl/err.h>

// Hàm in số Decimal
void print_bn_decimal(const char* label, const BIGNUM* bn) {
    if (bn == nullptr) {
        std::cout << label << ": (VAN BI NULL - Kiem tra lai file key)" << std::endl << std::endl;
        return;
    }
    char* dec_str = BN_bn2dec(bn);
    std::cout << label << " (" << BN_num_bits(bn) << " bits):" << std::endl;
    std::cout << dec_str << std::endl << std::endl;
    OPENSSL_free(dec_str);
}

int main() {
    const char* filename = "priv.pem";
    
    // 1. Mở file
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        std::cerr << "Khong the mo file priv.pem" << std::endl;
        return 1;
    }

    // 2. Đọc Key
    EVP_PKEY* pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);

    if (!pkey) {
        std::cerr << "Loi doc file PEM." << std::endl;
        return 1;
    }

    // 3. CHIẾN THUẬT MỚI: Ép kiểu về cấu trúc RSA cổ điển
    // Hàm này sẽ lôi "ruột" RSA ra khỏi lớp vỏ EVP_PKEY
    RSA* rsa = EVP_PKEY_get1_RSA(pkey);

    if (rsa == nullptr) {
        std::cerr << "Loi: Khong lay duoc cau truc RSA tu Key." << std::endl;
    } else {
        // 4. Khai báo các con trỏ để hứng dữ liệu
        const BIGNUM *n, *e, *d;
        const BIGNUM *p, *q;

        // Lấy n, e, d
        RSA_get0_key(rsa, &n, &e, &d);
        
        // Lấy p (prime1) và q (prime2)
        // Đây là hàm chuyên biệt để lấy thừa số nguyên tố
        RSA_get0_factors(rsa, &p, &q);

        std::cout << "--- KET QUA GIAI MA (DECIMAL) ---" << std::endl;
        
        print_bn_decimal("Prime 1 (p)", p);
        print_bn_decimal("Prime 2 (q)", q);
        print_bn_decimal("Modulus (n)", n);
        print_bn_decimal("Private Exponent (d)", d);

        // Giải phóng RSA (EVP_PKEY_get1_RSA tăng reference count nên phải free)
        RSA_free(rsa);
    }

    // 5. Dọn dẹp
    EVP_PKEY_free(pkey);
    return 0;
}