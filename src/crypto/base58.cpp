// Kasturisundari Chain — Base58 & Base58Check Implementation

#include "kasturisundari/crypto/base58.h"
#include "kasturisundari/crypto/sha256.h"
#include <algorithm>
#include <cstring>

namespace kasturisundari {
namespace crypto {

namespace {
const char BASE58_ALPHABET[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

constexpr int8_t BASE58_MAP[128] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15,16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29,30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39,40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54,55,56,57,-1,-1,-1,-1,-1
};
} // anonymous namespace

std::string base58_encode(const std::vector<uint8_t>& data) {
    size_t leading_zeros = 0;
    for (auto byte : data) {
        if (byte != 0) break;
        ++leading_zeros;
    }
    size_t size = (data.size() - leading_zeros) * 138 / 100 + 1;
    std::vector<uint8_t> b58(size, 0);
    for (size_t i = leading_zeros; i < data.size(); ++i) {
        int carry = data[i];
        for (auto it = b58.rbegin(); it != b58.rend(); ++it) {
            carry += 256 * (*it);
            *it = static_cast<uint8_t>(carry % 58);
            carry /= 58;
        }
    }
    auto it = b58.begin();
    while (it != b58.end() && *it == 0) ++it;
    std::string result(leading_zeros, '1');
    for (; it != b58.end(); ++it) {
        result += BASE58_ALPHABET[*it];
    }
    return result;
}

std::vector<uint8_t> base58_decode(const std::string& encoded) {
    if (encoded.empty()) return {};
    size_t leading_ones = 0;
    for (auto c : encoded) {
        if (c != '1') break;
        ++leading_ones;
    }
    size_t size = encoded.size() * 733 / 1000 + 1;
    std::vector<uint8_t> b256(size, 0);
    for (size_t i = leading_ones; i < encoded.size(); ++i) {
        char c = encoded[i];
        if (static_cast<unsigned char>(c) >= 128) return {};
        int digit = BASE58_MAP[static_cast<unsigned char>(c)];
        if (digit < 0) return {};
        int carry = digit;
        for (auto it = b256.rbegin(); it != b256.rend(); ++it) {
            carry += 58 * (*it);
            *it = static_cast<uint8_t>(carry % 256);
            carry /= 256;
        }
        if (carry != 0) return {};
    }
    auto it = b256.begin();
    while (it != b256.end() && *it == 0) ++it;
    std::vector<uint8_t> result(leading_ones, 0);
    result.insert(result.end(), it, b256.end());
    return result;
}

std::string base58check_encode(const std::vector<uint8_t>& payload) {
    Hash256 hash = double_sha256(payload.data(), payload.size());
    std::vector<uint8_t> data = payload;
    data.insert(data.end(), hash.begin(), hash.begin() + 4);
    return base58_encode(data);
}

std::vector<uint8_t> base58check_decode(const std::string& encoded) {
    std::vector<uint8_t> decoded = base58_decode(encoded);
    if (decoded.size() < 4) return {};
    std::vector<uint8_t> payload(decoded.begin(), decoded.end() - 4);
    std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());
    Hash256 hash = double_sha256(payload.data(), payload.size());
    if (std::memcmp(hash.data(), checksum.data(), 4) != 0) return {};
    return payload;
}

} // namespace crypto
} // namespace kasturisundari
