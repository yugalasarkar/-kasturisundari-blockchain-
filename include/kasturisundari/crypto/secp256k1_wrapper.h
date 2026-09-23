// Kasturisundari Chain — secp256k1 Elliptic Curve Wrapper
// Production-grade implementation using OpenSSL with RFC 6979 deterministic nonces,
// Low-S malleability protection, and public key recovery.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace kasturisundari {
namespace crypto {

/// A 32-byte private key.
using PrivateKey = std::array<uint8_t, 32>;

/// A 33-byte compressed public key.
using PublicKey = std::array<uint8_t, 33>;

/// A 64-byte compact ECDSA signature (r, s).
using Signature = std::array<uint8_t, 64>;

/// A 32-byte SHA-256 hash array.
using Hash256 = std::array<uint8_t, 32>;

/// Generate a cryptographically secure random private key using OpenSSL RAND_bytes.
PrivateKey generate_private_key();

/// Derive the compressed public key from a private key.
/// Returns std::nullopt if the private key is invalid.
std::optional<PublicKey> derive_public_key(const PrivateKey& privkey);

/// Sign a 32-byte message hash with a private key using RFC 6979 deterministic nonces
/// and enforcing canonical Low-S constraints.
/// Returns std::nullopt on failure.
std::optional<Signature> sign(const PrivateKey& privkey,
                              const Hash256& msg_hash);

/// Sign a 32-byte message hash and return both 64-byte signature and recovery ID (v: 0 or 1).
std::optional<Signature> sign_with_recovery(const PrivateKey& privkey,
                                            const Hash256& msg_hash,
                                            uint8_t& out_v);

/// Verify a signature against a message hash and public key with Low-S enforcement.
bool verify(const PublicKey& pubkey,
            const Hash256& msg_hash,
            const Signature& sig);

/// Recover a compressed public key from signature and recovery ID (v: 0 or 1, or 27/28).
std::optional<PublicKey> recover_public_key(const Hash256& msg_hash,
                                            const Signature& sig,
                                            uint8_t v);

/// Convert a private key to a hexadecimal string.
std::string privkey_to_hex(const PrivateKey& key);

/// Convert a public key to a hexadecimal string.
std::string pubkey_to_hex(const PublicKey& key);

/// Convert a signature to a hexadecimal string.
std::string sig_to_hex(const Signature& sig);

/// Parse hex string to PrivateKey.
std::optional<PrivateKey> hex_to_privkey(const std::string& hex);

/// Parse hex string to PublicKey.
std::optional<PublicKey> hex_to_pubkey(const std::string& hex);

/// Parse hex string to Signature.
std::optional<Signature> hex_to_sig(const std::string& hex);

} // namespace crypto
} // namespace kasturisundari
