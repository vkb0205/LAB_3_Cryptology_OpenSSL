#ifndef BIGINT_H
#define BIGINT_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>      // For uint64_t, uint32_t
#include <stdexcept>    // For std::runtime_error
#include <algorithm>    // For std::max, std::min
#include <iomanip>      // For std::setw, std::setfill

// --- FOR 128-BIT ARITHMETIC ---
#if defined(__GNUC__) || defined(__clang__)
using uint128_t = __uint128_t;
#else
// Fallback for MSVC or other compilers (much slower)
struct uint128_t {
    uint64_t lo, hi;
    uint128_t(uint64_t l = 0, uint64_t h = 0) : lo(l), hi(h) {}
    // Simplified constructor for this problem
    uint128_t(uint64_t l) : lo(l), hi(0) {}

    explicit operator uint64_t() const {
        return lo;
    };

    static inline uint128_t mul_u64(uint64_t a, uint64_t b) {
        unsigned __int64 hi;
        unsigned __int64 lo = _umul128(a, b, &hi); 
        return uint128_t(lo, hi);
    }

    // Helper to add a 128-bit and 64-bit integer
    static inline uint128_t add_u128_u64(uint128_t a, uint64_t b) {
        uint64_t new_lo = a.lo + b;
        uint64_t carry = (new_lo < a.lo); // 1 if overflow occurred
        return uint128_t(new_lo, a.hi + carry);
    }
};
#endif

/**
 * @brief The BigInt Class
 * Stores a signed integer as a vector of 64-bit "limbs" (magnitude) + a sign flag.
 * Limbs are little-endian (limbs[0] is the least significant).
 */
class BigInt {
public:
    std::vector<uint64_t> limbs; // magnitude (always non-negative)
    bool neg;                     // sign flag; false means non-negative, true means negative

    // --- Constructors ---
    BigInt() : neg(false) { limbs.push_back(0); }
    BigInt(int64_t v) : neg(v < 0) {
        if (v < 0) {
            neg = true;
            limbs.push_back(-v);
        } else {
            neg = false;
            limbs.push_back(v);
        }
        normalize();
    }

    // Constructor from a big-endian hex string (unsigned interpretation)
    BigInt(const std::string& hex_str) : neg(false) {
        if (hex_str.empty()) {
            limbs.push_back(0);
            return;
        }
        // Each hex char is 4 bits. 16 hex chars = 64 bits (one limb).
        int len = hex_str.length();
        int num_limbs = (len + 15) / 16;
        limbs.resize(num_limbs, 0);

        int limb_idx = 0;
        int count = 0;
        uint64_t current_limb = 0;

        //Since the string stores the number in the format of big-endian
        for (int i = len - 1; i >= 0; --i) {
            char c = hex_str[i];
            uint64_t val;
            if (c >= '0' && c <= '9') val = c - '0';
            else if (c >= 'a' && c <= 'f') 
                val = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') 
                val = c - 'A' + 10;
            else 
                throw std::runtime_error("Invalid hex character");

            current_limb |= (val << (count * 4));
            count++;
            if (count == 16) {
                limbs[limb_idx++] = current_limb;
                current_limb = 0;
                count = 0;
            }
        }
        if (count > 0) {
            limbs[limb_idx] = current_limb;
        }
        normalize();
    }

    std::string to_hex_string() const {
        std::ostringstream oss;
        oss << std::hex << std::nouppercase;
        if (limbs.empty()) return "0";
        if (neg) oss << "-";
        oss << limbs.back();
        for (size_t i = limbs.size(); i-- > 1;) {
            oss << std::setw(16) << std::setfill('0') << limbs[i - 1];
        }
        return oss.str();
    }

    // --- Helper Functions ---
    static unsigned count_leading_zeros(uint64_t limb) {
    #if defined(_MSC_VER) && !defined(__clang__)
        unsigned long idx;
        _BitScanReverse64(&idx, limb);
        return 63u - idx;
    #else
        return limb ? static_cast<unsigned>(__builtin_clzll(limb)) : 64u;
    #endif
    }

    size_t bit_length() const {
        if (is_zero()) return 0;
        size_t ms = limbs.size() - 1;
        uint64_t v = limbs[ms];
        return ms * 64 + (64 - count_leading_zeros(v));
    }

    void normalize() {
        while (limbs.size() > 1 && limbs.back() == 0) limbs.pop_back(); //Trimming leading zero limbs
        if (limbs.size() == 1 && limbs[0] == 0) neg = false;
    }
    
    bool is_zero() const {
        return limbs.size() == 1 && limbs[0] == 0;
    }

    bool is_even() const {
        return (limbs[0] & 1) == 0;
    }

    BigInt abs() const {
        return BigInt(*this).set_positive();
    }

    BigInt& set_positive() {
        neg = false;
        return *this;
    }
    
    BigInt& set_negative() {
        if (!is_zero()) neg = true;
        return *this;
    }

    // --- Unary Minus ---
    BigInt operator-() const {
        BigInt result = *this;
        if (!result.is_zero()) result.neg = !result.neg;
        return result;
    }

    // --- Comparison Operators ---
    friend bool operator==(const BigInt& a, const BigInt& b) {
        return a.limbs == b.limbs && a.neg == b.neg;
    }
    friend bool operator!=(const BigInt& a, const BigInt& b) {
        return !(a == b);
    }
    friend bool operator<(const BigInt& a, const BigInt& b) {
        if (a.neg != b.neg) return a.neg > b.neg;
        
        bool less_unsigned = false;
        if (a.limbs.size() != b.limbs.size()) {
            less_unsigned = (a.limbs.size() < b.limbs.size());
        }
        else {
            if (a.limbs == b.limbs) return false;
            for (int i = a.limbs.size() - 1; i >= 0; --i) {
                if (a.limbs[i] != b.limbs[i]) {
                    less_unsigned = a.limbs[i] < b.limbs[i];
                    break;
                }
            }
        }
        return a.neg ? !less_unsigned : less_unsigned;
    }
    friend bool operator>(const BigInt& a, const BigInt& b) { return b < a; }
    friend bool operator<=(const BigInt& a, const BigInt& b) { return !(a > b); }
    friend bool operator>=(const BigInt& a, const BigInt& b) { return !(a < b); }

    // --- Bitwise Shift Operators ---
    friend BigInt operator<<(const BigInt& a, size_t shift_bits) {
        BigInt result = a;
        size_t shift_limbs = shift_bits / 64;
        size_t inner_shift = shift_bits % 64;

        if (inner_shift > 0) {
            uint64_t carry = 0;
            for (size_t i = 0; i < result.limbs.size(); ++i) {
                uint64_t next_carry = result.limbs[i] >> (64 - inner_shift); //Take the overflow when you shift left 
                result.limbs[i] = (result.limbs[i] << inner_shift) | carry; //Store the rest except the overflow part into the current limb
                carry = next_carry; //Take the overflow part to the next limb
            }
            if (carry > 0) 
                result.limbs.push_back(carry);
        }
        if (shift_limbs > 0) {
            result.limbs.insert(result.limbs.begin(), shift_limbs, 0); //Insert 0s to the least significant location
        }
        result.normalize();
        return result;
    }
    friend BigInt operator>>(const BigInt& a, size_t shift_bits) {
        BigInt result = a;
        size_t shift_limbs = shift_bits / 64;
        size_t inner_shift = shift_bits % 64;

        if (shift_limbs >= result.limbs.size()) return BigInt(0);
        if (shift_limbs > 0) {
            result.limbs.erase(result.limbs.begin(), result.limbs.begin() + shift_limbs);
        }
        if (inner_shift > 0) {
            uint64_t borrow = 0;
            for (int i = result.limbs.size() - 1; i >= 0; --i) {
                uint64_t next_borrow = result.limbs[i] << (64 - inner_shift);
                result.limbs[i] = (result.limbs[i] >> inner_shift) | borrow;
                borrow = next_borrow;
            }
        }
        result.normalize();
        return result;
    }

    // --- Arithmetic Operators ---
    friend BigInt operator+(const BigInt& a, const BigInt& b) {
        if (a.neg == b.neg) {
            // Same sign: add magnitudes
            BigInt result;
            result.limbs.clear();
            
            size_t n = std::max(a.limbs.size(), b.limbs.size());
            result.limbs.resize(n);
            
            uint64_t carry = 0;
            for (size_t i = 0; i < n; ++i) {
               //Take each set of 64-bit in each limb, if exceed the smaller bigint, refresh the index of that to 0 and go on
                uint64_t a_limb = (i < a.limbs.size()) ? a.limbs[i] : 0;
                uint64_t b_limb = (i < b.limbs.size()) ? b.limbs[i] : 0;
                
                // This detects overflow: if sum < a_limb, it wrapped around
                uint64_t sum = a_limb + carry;
                bool carry1 = (sum < a_limb);
                
                sum += b_limb;
                bool carry2 = (sum < b_limb);
                
                result.limbs[i] = sum;
                carry = (carry1 || carry2) ? 1 : 0;
            }
            if (carry > 0) {
                result.limbs.push_back(carry);
            }
            result.neg = a.neg;
            result.normalize();
            return result;
        } else {
            // Different signs: subtract magnitudes
            if (a.limbs == b.limbs) {
                return BigInt(0);
            }

            bool a_ge_b;
            if (a.limbs.size() != b.limbs.size()) {
                a_ge_b = (a.limbs.size() > b.limbs.size());
            } else {
                // same size -> compare limb-by-limb from most significant
                a_ge_b = false;
                for (int i = a.limbs.size() - 1; i >= 0; --i) {
                    if (a.limbs[i] != b.limbs[i]) {
                        a_ge_b = (a.limbs[i] > b.limbs[i]);
                        break;
                    }
                }
            }

            BigInt result;
            const BigInt *larger = a_ge_b ? &a : &b;
            const BigInt *smaller = a_ge_b ? &b : &a;
            result.limbs.assign(larger->limbs.size(), 0);

            uint64_t borrow = 0;
            for (size_t i = 0; i < larger->limbs.size(); ++i) {
                uint64_t li = (i < larger->limbs.size()) ? larger->limbs[i] : 0;
                uint64_t si = (i < smaller->limbs.size()) ? smaller->limbs[i] : 0;
                uint64_t sub = li;
                if (sub < si + borrow) {
                    sub = sub + (UINT64_MAX - si - borrow + 1);
                    borrow = 1;
                } else {
                    sub = sub - si - borrow;
                    borrow = 0;
                }
                result.limbs[i] = sub;
            }

            result.neg = larger->neg; // sign follows the operand with larger magnitude
            result.normalize();
            return result;
        }
    }

    friend BigInt operator-(const BigInt& a, const BigInt& b) {
        return a + (-b);
    }

    friend BigInt operator*(const BigInt& a, const BigInt& b) {
        if (a.is_zero() || b.is_zero()) return BigInt(0);
        
        BigInt result;
        result.limbs.assign(a.limbs.size() + b.limbs.size(), 0);

        for (size_t i = 0; i < a.limbs.size(); ++i) {
            uint64_t carry = 0;
            for (size_t j = 0; j < b.limbs.size(); ++j) {
                #if defined(__GNUC__) || defined(__clang__)
                // GCC/Clang path using native 128-bit type
                __uint128_t product = (__uint128_t)a.limbs[i] * b.limbs[j] + 
                                      result.limbs[i + j] + carry;
                
                result.limbs[i + j] = (uint64_t)product;
                carry = (uint64_t)(product >> 64);
                #else
                // MSVC path using the struct and static helpers
                uint128_t product = uint128_t::mul_u64(a.limbs[i], b.limbs[j]);
                product = uint128_t::add_u128_u64(product, result.limbs[i + j]);
                product = uint128_t::add_u128_u64(product, carry);

                result.limbs[i + j] = product.lo;
                carry = product.hi;
                #endif
            }
            if (carry > 0) {
                result.limbs[i + b.limbs.size()] += carry;
            }
        }
        result.neg = (a.neg != b.neg) && !result.is_zero();
        result.normalize();
        return result;
    }

    static std::pair<BigInt, BigInt> divmod(const BigInt& dividend_in, const BigInt& divisor_in) {
        if (divisor_in.is_zero()) {
            throw std::invalid_argument("Division by zero");
        }

        // Special-case small dividend < divisor: quotient = 0, remainder = dividend (keep sign)
        BigInt u = dividend_in.abs();
        BigInt v = divisor_in.abs();
        if (u < v) {
            BigInt q(0);
            BigInt r = dividend_in; // keep original sign for remainder
            return {q, r};
        }

        // Prepare quotient with enough limbs to hold (shift + 1) bits
        size_t shift = u.bit_length() - v.bit_length(); // >= 0
        size_t q_bits = shift + 1;
        size_t q_limbs = (q_bits + 63) / 64;
        BigInt quotient;
        quotient.limbs.assign(q_limbs, 0);

        BigInt remainder = u;
        BigInt shifted = v << shift;

        for (size_t i = shift + 1; i-- > 0;) {
            if (!(remainder < shifted)) {
                remainder = remainder - shifted;
                size_t limb_idx = i / 64;
                size_t bit_idx = i % 64;
                if (limb_idx >= quotient.limbs.size()) quotient.limbs.resize(limb_idx + 1, 0);
                quotient.limbs[limb_idx] |= (1ULL << bit_idx);
            }
            shifted = shifted >> 1;
        }

        quotient.normalize();
        remainder.normalize();

        // Apply signs
        bool q_neg = (dividend_in.neg != divisor_in.neg) && !quotient.is_zero();
        bool r_neg = dividend_in.neg && !remainder.is_zero();
        quotient.neg = q_neg;
        remainder.neg = r_neg;

        return {quotient, remainder};
    }

    friend BigInt operator/(const BigInt& a, const BigInt& b) {
        return divmod(a, b).first;
    }
    friend BigInt operator%(const BigInt& a, const BigInt& b) {
        return divmod(a, b).second;
    }

    // --- Compound Assignment ---
    BigInt& operator+=(const BigInt& o) { *this = *this + o; return *this; }
    BigInt& operator-=(const BigInt& o) { *this = *this - o; return *this; }
    BigInt& operator*=(const BigInt& o) { *this = *this * o; return *this; }
    BigInt& operator/=(const BigInt& o) { *this = *this / o; return *this; }
    BigInt& operator%=(const BigInt& o) { *this = *this % o; return *this; }
};

#endif // BIGINT_H
