// Kasturisundari Chain — RIPEMD-160 Cryptographic Hash
// Used in address generation: RIPEMD160(SHA256(pubkey))

#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace kasturisundari {
namespace crypto {

/// 160-bit (20-byte) hash digest.
using Hash160 = std::array<uint8_t, 20>;

/// Compute the RIPEMD-160 hash of arbitrary binary data.
Hash160 ripemd160(const uint8_t* data, size_t length);

/// Convenience overload for std::vector.
Hash160 ripemd160(const std::vector<uint8_t>& data);

} // namespace crypto
} // namespace kasturisundari
