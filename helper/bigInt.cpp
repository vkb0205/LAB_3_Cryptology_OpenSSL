#include "bigInt.h"

// --- Main Program ---
int main() {
    std::string str1 = readFile("test\\project_01_01\\test_00.inp");
    std::string str2 = readFile("test\\project_01_01\\test_01.inp");
    if (str1.empty()) {
        std::cerr << "Error: test.inp is empty." << std::endl;
        return 1;
    }
    BigInt n(parse_little_endian_hex(str1));
    BigInt m(parse_little_endian_hex(str2));
    n.print_hex();
    m.print_hex();
    std::cout << "Result: ";
    n = n - m;
    n.print_hex();

    return 0;
}