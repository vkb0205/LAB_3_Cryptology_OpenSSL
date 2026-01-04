# LAB 3 — OpenSSL Exercises (Ex1, Ex2, Ex3)

This folder collects three small OpenSSL-based exercises used in the lab:

- Ex1: `key_analysis.cpp` — inspect RSA key parameters (modulus, exponents, primes) from PEM files.
- Ex2: `encrypt.cpp` / `decrypt.cpp` — RSA encrypt/decrypt small files using `EVP_PKEY_encrypt` / `EVP_PKEY_decrypt`.
- Ex3: `sign.cpp` / `verify.cpp` — create and verify RSA signatures using SHA-256 with `EVP_DigestSign` / `EVP_DigestVerify`.

See the per-exercise README files for detailed instructions:

- [Ex1 README](Cryptology/LAB_3/git_repo/LAB_3_Cryptology_OpenSSL/Source/Ex1/README.md)
- [Ex2 README](Cryptology/LAB_3/git_repo/LAB_3_Cryptology_OpenSSL/Source/Ex2/README.md)
- [Ex3 README](Cryptology/LAB_3/git_repo/LAB_3_Cryptology_OpenSSL/Source/Ex3/README.md)

## Prerequisites

- OpenSSL 3.x (development headers and libraries)
- A C++17-capable compiler (g++, clang, or MSVC)
- `pkg-config` recommended on Unix-like systems for easy linking

## Quick setup (generate test keys)
Use these openssl commands to create a test RSA keypair used by the examples:

```bash
openssl genpkey -algorithm RSA -out priv.pem -pkeyopt rsa_keygen_bits:2048
openssl rsa -pubout -in priv.pem -out pub.pem
```

Place `priv.pem` and `pub.pem` in the same folder where you run the example programs, or provide full paths to them when running.

## Build all examples (Unix-like, with pkg-config)
```bash
g++ -std=c++17 Ex1/key_analysis.cpp -o key_analysis $(pkg-config --cflags --libs openssl)
g++ -std=c++17 Ex2/encrypt.cpp -o encrypt $(pkg-config --cflags --libs openssl)
g++ -std=c++17 Ex2/decrypt.cpp -o decrypt $(pkg-config --cflags --libs openssl)
g++ -std=c++17 Ex3/sign.cpp -o sign $(pkg-config --cflags --libs openssl)
g++ -std=c++17 Ex3/verify.cpp -o verify $(pkg-config --cflags --libs openssl)
```

## Simple usage examples
**NOTE: ** Make sure you built the `.cpp` programs listed above in the `Source` directory. 
- Inspect key parameters (Ex1):
```bash
./key_analysis priv.pem pub.pem
```

- Encrypt / Decrypt a small file (Ex2):
```bash
./encrypt pub.pem txt/message.txt cipher.bin
./decrypt priv.pem cipher.bin recovered.txt
```

- Sign / Verify a file (Ex3):
```bash
./sign priv.pem txt/message.txt signature.bin
./verify pub.pem txt/message.txt signature.bin
```

## Notes

- These examples use raw RSA operations (PKCS#1 v1.5 padding for Ex2) and are intended for learning/demonstration. For production use, prefer hybrid encryption (RSA + AES/GCM) and modern signature schemes or proper protocols.
- If you encounter linker/header errors, confirm your OpenSSL installation (matching headers/libs), and that the examples are built against OpenSSL 3.x because some code paths rely on newer EVP APIs.
