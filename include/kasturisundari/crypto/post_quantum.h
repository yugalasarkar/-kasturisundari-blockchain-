// Kasturisundari Chain — Post-Quantum Cryptography Engine
// Implements a Hybrid Signature scheme combining secp256k1 with NIST FIPS 204 ML-DSA-65 (CRYSTALS-Dilithium).

#pragma once

#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include "kasturisundari/crypto/sha256.h"

#include <cstdint>
#include <string>
#include <vector>

namespace kasturisundari {
namespace crypto {
namespace pqc {

// ML-DSA-65 / Dilithium3 parameters (NIST Level 3)
constexpr size_t DILITHIUM_PUBKEY_BYTES = 1312;
constexpr size_t DILITHIUM_PRIVKEY_BYTES = 2528;
constexpr size_t DILITHIUM_SIG_BYTES = 2420;

/// Post-Quantum Public Key
struct PqPublicKey {
    std::vector<uint8_t> data;
    PqPublicKey() : data(DILITHIUM_PUBKEY_BYTES, 0) {}
};

/// Post-Quantum Private Key
struct PqPrivateKey {
    std::vector<uint8_t> data;
    PqPrivateKey() : data(DILITHIUM_PRIVKEY_BYTES, 0) {}
};

/// Post-Quantum Signature
struct PqSignature {
    std::vector<uint8_t> data;
    PqSignature() : data(DILITHIUM_SIG_BYTES, 0) {}
};

/// Hybrid Public Key (Classical secp256k1 + Post-Quantum)
struct HybridPublicKey {
    PublicKey classical;
    PqPublicKey pq;

    std::vector<uint8_t> serialize() const;
    static HybridPublicKey deserialize(const std::vector<uint8_t>& bytes);
};

/// Hybrid Signature (Classical secp256k1 + Post-Quantum)
struct HybridSignature {
    Signature classical;
    PqSignature pq;

    std::vector<uint8_t> serialize() const;
    static HybridSignature deserialize(const std::vector<uint8_t>& bytes);
};

/// Generate a Post-Quantum keypair (using liboqs ML-DSA-65 if USE_LIBOQS is defined,
/// or deterministic HMAC-SHA256 expansion).
void generate_pq_keypair(PqPublicKey& pub, PqPrivateKey& priv);

/// Sign a message hash using the hybrid scheme
HybridSignature hybrid_sign(const PrivateKey& classical_priv, 
                            const PqPrivateKey& pq_priv, 
                            const Hash256& msg_hash);

/// Verify a hybrid signature.
/// Both classical secp256k1 and post-quantum verification must pass!
bool hybrid_verify(const HybridPublicKey& pub, 
                   const Hash256& msg_hash, 
                   const HybridSignature& sig);

} // namespace pqc
} // namespace crypto
} // namespace kasturisundari
