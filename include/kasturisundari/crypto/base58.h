// Kasturisundari Chain — Base58 & Base58Check Encoding
// Used for human-readable address encoding (like Bitcoin addresses).

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kasturisundari {
namespace crypto {

/// Encode raw bytes into a Base58 string.
std::string base58_encode(const std::vector<uint8_t>& data);

/// Decode a Base58 string back to raw bytes.
/// Returns an empty vector on invalid input.
std::vector<uint8_t> base58_decode(const std::string& encoded);

/// Encode raw bytes with a 4-byte checksum appended (Base58Check).
std::string base58check_encode(const std::vector<uint8_t>& payload);

/// Decode a Base58Check string, verifying the checksum.
/// Returns an empty vector if the checksum is invalid.
std::vector<uint8_t> base58check_decode(const std::string& encoded);

} // namespace crypto
} // namespace kasturisundari
