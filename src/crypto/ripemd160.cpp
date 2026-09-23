// Kasturisundari Chain — RIPEMD-160 Implementation
// Used in address generation: RIPEMD160(SHA256(pubkey))
// Based on the original RIPEMD-160 specification.

#include "kasturisundari/crypto/ripemd160.h"

#include <cstring>

namespace kasturisundari {
namespace crypto {

namespace {

inline uint32_t rotl(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32 - n));
}

inline uint32_t f(unsigned j, uint32_t x, uint32_t y, uint32_t z) {
    if (j < 16) return x ^ y ^ z;
    if (j < 32) return (x & y) | (~x & z);
    if (j < 48) return (x | ~y) ^ z;
    if (j < 64) return (x & z) | (y & ~z);
    return x ^ (y | ~z);
}

constexpr uint32_t KL[5] = {
    0x00000000, 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xa953fd4e
};
constexpr uint32_t KR[5] = {
    0x50a28be6, 0x5c4dd124, 0x6d703ef3, 0x7a6d76e9, 0x00000000
};

constexpr unsigned RL[80] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8,
    3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11, 5, 12,
    1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2,
    4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13
};

constexpr unsigned RR[80] = {
    5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12,
    6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2,
    15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13,
    8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14,
    12, 15, 10, 4, 1, 5, 8, 7, 6, 2, 13, 14, 0, 3, 9, 11
};

constexpr unsigned SL[80] = {
    11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8,
    7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12,
    11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5,
    11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12,
    9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6
};

constexpr unsigned SR[80] = {
    8, 9, 9, 11, 13, 15, 15, 5, 7, 7, 8, 11, 14, 14, 12, 6,
    9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11,
    9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5,
    15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8,
    8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11
};

void ripemd160_transform(uint32_t state[5], const uint8_t block[64]) {
    uint32_t x[16];
    for (int i = 0; i < 16; ++i) {
        x[i] = static_cast<uint32_t>(block[i * 4])
             | (static_cast<uint32_t>(block[i * 4 + 1]) << 8)
             | (static_cast<uint32_t>(block[i * 4 + 2]) << 16)
             | (static_cast<uint32_t>(block[i * 4 + 3]) << 24);
    }

    uint32_t al = state[0], bl = state[1], cl = state[2], dl = state[3], el = state[4];
    uint32_t ar = state[0], br = state[1], cr = state[2], dr = state[3], er = state[4];

    for (unsigned j = 0; j < 80; ++j) {
        unsigned group = j / 16;
        uint32_t tl = rotl(al + f(j, bl, cl, dl) + x[RL[j]] + KL[group], SL[j]) + el;
        al = el; el = dl; dl = rotl(cl, 10); cl = bl; bl = tl;

        uint32_t tr = rotl(ar + f(79 - j, br, cr, dr) + x[RR[j]] + KR[group], SR[j]) + er;
        ar = er; er = dr; dr = rotl(cr, 10); cr = br; br = tr;
    }

    uint32_t t = state[1] + cl + dr;
    state[1] = state[2] + dl + er;
    state[2] = state[3] + el + ar;
    state[3] = state[4] + al + br;
    state[4] = state[0] + bl + cr;
    state[0] = t;
}

} // anonymous namespace

Hash160 ripemd160(const uint8_t* data, size_t length) {
    uint32_t state[5] = {
        0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
    };

    size_t offset = 0;
    while (offset + 64 <= length) {
        ripemd160_transform(state, data + offset);
        offset += 64;
    }

    // Padding.
    uint8_t buffer[128];
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

    // Append length in bits as 64-bit little-endian.
    uint64_t bit_len = static_cast<uint64_t>(length) * 8;
    for (int i = 0; i < 8; ++i) {
        buffer[pad_len - 8 + i] = static_cast<uint8_t>(bit_len >> (i * 8));
    }

    for (size_t i = 0; i < pad_len; i += 64) {
        ripemd160_transform(state, buffer + i);
    }

    Hash160 result;
    for (int i = 0; i < 5; ++i) {
        result[i * 4]     = static_cast<uint8_t>(state[i]);
        result[i * 4 + 1] = static_cast<uint8_t>(state[i] >> 8);
        result[i * 4 + 2] = static_cast<uint8_t>(state[i] >> 16);
        result[i * 4 + 3] = static_cast<uint8_t>(state[i] >> 24);
    }
    return result;
}

Hash160 ripemd160(const std::vector<uint8_t>& data) {
    return ripemd160(data.data(), data.size());
}

} // namespace crypto
} // namespace kasturisundari
