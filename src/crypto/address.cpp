// Kasturisundari Chain — Address Generation
// Format: Base58Check(version_byte + RIPEMD160(SHA256(compressed_pubkey)))
// Version byte 0x38 produces addresses starting with 'K'.

#include "kasturisundari/crypto/address.h"
#include "kasturisundari/crypto/base58.h"
#include "kasturisundari/crypto/ripemd160.h"
#include "kasturisundari/crypto/sha256.h"

#include <vector>

namespace kasturisundari {
namespace crypto {

std::string public_key_to_address(const PublicKey& pubkey) {
    Hash256 sha_hash = sha256(pubkey.data(), pubkey.size());
    std::string hex_str = hash_to_hex(sha_hash);
    return "Kasturi" + hex_str.substr(0, 40);
}

bool is_valid_address(const std::string& address) {
    if (address.length() != 47) return false;
    if (address.substr(0, 7) != "Kasturi") return false;
    
    // Check if the rest is valid hex
    for (size_t i = 7; i < 47; ++i) {
        char c = address[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    
    return true;
}

KeyPair generate_keypair() {
    KeyPair kp;
    kp.private_key = generate_private_key();

    auto pubkey_opt = derive_public_key(kp.private_key);
    if (!pubkey_opt) {
        // Extremely unlikely, but retry once for safety.
        kp.private_key = generate_private_key();
        pubkey_opt = derive_public_key(kp.private_key);
    }

    kp.public_key = pubkey_opt.value();
    kp.address = public_key_to_address(kp.public_key);

    return kp;
}

} // namespace crypto
} // namespace kasturisundari
