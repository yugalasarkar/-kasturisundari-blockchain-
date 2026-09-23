// Kasturisundari Chain — Network Address Generation
// Derives a unique Kasturisundari address from a public key.
// Format: KS + Base58Check(version_byte + RIPEMD160(SHA256(pubkey)))

#pragma once

#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include <string>

namespace kasturisundari {
namespace crypto {

/// The version byte prepended to the public key hash before Base58Check.
/// 0x38 produces addresses starting with 'K'.
constexpr uint8_t ADDRESS_VERSION_BYTE = 0x2D;

/// Generate a Kasturisundari Chain address from a compressed public key.
/// The address format is: Base58Check(version_byte + RIPEMD160(SHA256(pubkey)))
std::string public_key_to_address(const PublicKey& pubkey);

/// Validate whether a string is a well-formed Kasturisundari address.
bool is_valid_address(const std::string& address);

/// A complete keypair with its derived address — the "wallet identity".
struct KeyPair {
    PrivateKey private_key;
    PublicKey  public_key;
    std::string address;
};

/// Generate a brand-new random keypair and its derived address.
KeyPair generate_keypair();

} // namespace crypto
} // namespace kasturisundari
