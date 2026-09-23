// Kasturisundari Chain — SHA-256 Implementation
// Based on FIPS 180-4. Pure C++, zero external dependencies.

#include "kasturisundari/crypto/sha256.h"

#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace kasturisundari {
namespace crypto {

namespace {

// SHA-256 initial hash values (first 32 bits of the fractional parts
// of the square roots of the first 8 primes).
constexpr uint32_t H_INIT[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

// SHA-256 round constants (first 32 bits of the fractional parts
// of the cube roots of the first 64 primes).
constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline uint32_t rotr(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32 - n));
}

inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline uint32_t sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline uint32_t gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline uint32_t gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];

    // Prepare the message schedule.
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(block[i * 4]) << 24)
             | (static_cast<uint32_t>(block[i * 4 + 1]) << 16)
             | (static_cast<uint32_t>(block[i * 4 + 2]) << 8)
             | (static_cast<uint32_t>(block[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
    }

    // Initialize working variables.
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    // 64 rounds of compression.
    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
        uint32_t t2 = sigma0(a) + maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

} // anonymous namespace

Hash256 sha256(const uint8_t* data, size_t length) {
    uint32_t state[8];
    std::memcpy(state, H_INIT, sizeof(H_INIT));

    // Process full 64-byte blocks.
    size_t offset = 0;
    while (offset + 64 <= length) {
        sha256_transform(state, data + offset);
        offset += 64;
    }

    // Padding: remaining bytes + 0x80 + zeros + 8-byte big-endian bit length.
    uint8_t buffer[128]; // At most 2 blocks needed for padding.
    size_t remaining = length - offset;
    std::memcpy(buffer, data + offset, remaining);
    buffer[remaining] = 0x80;

    size_t pad_len;
    if (remaining < 56) {
        pad_len = 64;
    } else {
        pad_len = 128;
    }
    std::memset(buffer + remaining + 1, 0, pad_len - remaining - 1);

    // Append the original message length in bits as a 64-bit big-endian.
    uint64_t bit_len = static_cast<uint64_t>(length) * 8;
    for (int i = 0; i < 8; ++i) {
        buffer[pad_len - 1 - i] = static_cast<uint8_t>(bit_len >> (i * 8));
    }

    // Process remaining blocks.
    for (size_t i = 0; i < pad_len; i += 64) {
        sha256_transform(state, buffer + i);
    }

    // Produce the final hash.
    Hash256 result;
    for (int i = 0; i < 8; ++i) {
        result[i * 4]     = static_cast<uint8_t>(state[i] >> 24);
        result[i * 4 + 1] = static_cast<uint8_t>(state[i] >> 16);
        result[i * 4 + 2] = static_cast<uint8_t>(state[i] >> 8);
        result[i * 4 + 3] = static_cast<uint8_t>(state[i]);
    }
    return result;
}

Hash256 sha256(const std::vector<uint8_t>& data) {
    return sha256(data.data(), data.size());
}

Hash256 sha256(const std::string& data) {
    return sha256(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

Hash256 double_sha256(const uint8_t* data, size_t length) {
    Hash256 first = sha256(data, length);
    return sha256(first.data(), first.size());
}

Hash256 double_sha256(const std::vector<uint8_t>& data) {
    return double_sha256(data.data(), data.size());
}

std::string hash_to_hex(const Hash256& hash) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto byte : hash) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

Hash256 hex_to_hash(const std::string& hex) {
    Hash256 result{};
    if (hex.size() != 64) return result;
    for (size_t i = 0; i < 32; ++i) {
        unsigned int byte_val = 0;
        std::istringstream(hex.substr(i * 2, 2)) >> std::hex >> byte_val;
        result[i] = static_cast<uint8_t>(byte_val);
    }
    return result;
}

} // namespace crypto
} // namespace kasturisundari
