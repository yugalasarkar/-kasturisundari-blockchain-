// Kasturisundari Chain — SHA-256 Cryptographic Hash
// Pure C++ implementation. No external dependencies.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace kasturisundari {
namespace crypto {

/// 256-bit (32-byte) hash digest.
using Hash256 = std::array<uint8_t, 32>;

/// Compute the SHA-256 hash of arbitrary binary data.
Hash256 sha256(const uint8_t* data, size_t length);

/// Convenience overload for std::vector.
Hash256 sha256(const std::vector<uint8_t>& data);

/// Convenience overload for std::string.
Hash256 sha256(const std::string& data);

/// Double SHA-256 (SHA-256 of SHA-256), used for block hashing.
Hash256 double_sha256(const uint8_t* data, size_t length);
Hash256 double_sha256(const std::vector<uint8_t>& data);

/// Convert a Hash256 to a lowercase hexadecimal string.
std::string hash_to_hex(const Hash256& hash);

/// Convert a hexadecimal string to a Hash256. Returns zeros on invalid input.
Hash256 hex_to_hash(const std::string& hex);

} // namespace crypto
} // namespace kasturisundari
