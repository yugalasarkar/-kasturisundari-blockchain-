// Kasturisundari Chain — Phase 4 Post-Quantum Tests
// Tests the Hybrid Signature system (secp256k1 + Dilithium)

#include "kasturisundari/crypto/post_quantum.h"
#include "kasturisundari/crypto/sha256.h"

#include <iostream>
#include <string>
#include <vector>

using namespace kasturisundari;
using namespace kasturisundari::crypto;

#define GREEN "\033[32m"
#define RED   "\033[31m"
#define CYAN  "\033[36m"
#define RESET "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    std::cout << CYAN "  [TEST] " RESET << name << "... ";

#define PASS() \
    do { std::cout << GREEN "PASSED" RESET << std::endl; ++tests_passed; } while(0)

#define FAIL(msg) \
    do { std::cout << RED "FAILED: " << msg << RESET << std::endl; ++tests_failed; } while(0)

void test_post_quantum() {
    std::cout << "\n=== Post-Quantum Hybrid Signature Tests ===" << std::endl;

    // 1. Setup classical keypair
    PrivateKey class_priv = generate_private_key();
    auto class_pub_opt = derive_public_key(class_priv);
    if (!class_pub_opt) { FAIL("failed classical keygen"); return; }
    PublicKey class_pub = *class_pub_opt;

    // 2. Setup Post-Quantum keypair
    pqc::PqPublicKey pq_pub;
    pqc::PqPrivateKey pq_priv;
    pqc::generate_pq_keypair(pq_pub, pq_priv);

    TEST("PQ Key generation sizes are strictly enforced (Dilithium2)");
    if (pq_pub.data.size() == pqc::DILITHIUM_PUBKEY_BYTES && 
        pq_priv.data.size() == pqc::DILITHIUM_PRIVKEY_BYTES) {
        PASS();
    } else {
        FAIL("invalid sizes");
    }

    // 3. Create Hybrid Keys
    pqc::HybridPublicKey hybrid_pub;
    hybrid_pub.classical = class_pub;
    hybrid_pub.pq = pq_pub;

    TEST("Hybrid Public Key Serialization");
    auto serialized_pub = hybrid_pub.serialize();
    if (serialized_pub.size() == 33 + pqc::DILITHIUM_PUBKEY_BYTES) {
        PASS();
    } else {
        FAIL("serialization size mismatch");
    }

    pqc::HybridPublicKey recovered_pub = pqc::HybridPublicKey::deserialize(serialized_pub);
    if (recovered_pub.classical == hybrid_pub.classical && recovered_pub.pq.data == hybrid_pub.pq.data) {
        PASS();
    } else {
        FAIL("deserialization mismatch");
    }

    // 4. Sign a message
    Hash256 msg_hash = sha256("Quantum safe message from Kasturisundari!");
    
    pqc::HybridSignature hybrid_sig = pqc::hybrid_sign(class_priv, pq_priv, msg_hash);

    TEST("Hybrid Signature Serialization");
    auto serialized_sig = hybrid_sig.serialize();
    if (serialized_sig.size() == 64 + pqc::DILITHIUM_SIG_BYTES) {
        PASS();
    } else {
        FAIL("signature serialization size mismatch");
    }

    // 5. Verify the signature
    TEST("Hybrid Signature Validates correctly");
    bool is_valid = pqc::hybrid_verify(hybrid_pub, msg_hash, hybrid_sig);
    if (is_valid) {
        PASS();
    } else {
        FAIL("valid signature rejected");
    }

    // 6. Test Tampering
    TEST("Reject if classical signature is tampered (Shor's algorithm scenario)");
    pqc::HybridSignature tampered_classical = hybrid_sig;
    tampered_classical.classical[5] ^= 0xFF; // flip bits
    if (!pqc::hybrid_verify(hybrid_pub, msg_hash, tampered_classical)) {
        PASS();
    } else {
        FAIL("accepted broken classical sig");
    }

    TEST("Reject if PQ signature is tampered (Classical brute force scenario)");
    pqc::HybridSignature tampered_pq = hybrid_sig;
    tampered_pq.pq.data[100] ^= 0xFF;
    if (!pqc::hybrid_verify(hybrid_pub, msg_hash, tampered_pq)) {
        PASS();
    } else {
        FAIL("accepted broken pq sig");
    }

    TEST("Reject if message hash changes");
    Hash256 fake_hash = sha256("Fake message");
    if (!pqc::hybrid_verify(hybrid_pub, fake_hash, hybrid_sig)) {
        PASS();
    } else {
        FAIL("accepted different message");
    }
    
    std::cout << "\n--- Security Summary ---" << std::endl;
    std::cout << "  Classical Curve: secp256k1 (256-bit)" << std::endl;
    std::cout << "  Post-Quantum:    CRYSTALS-Dilithium2 (NIST FIPS 204 Level 2)" << std::endl;
    std::cout << "  Hybrid Scheme:   Active (Dual Verification)" << std::endl;
    std::cout << "  Signature Size:  " << serialized_sig.size() << " bytes" << std::endl;
    std::cout << "  Public Key Size: " << serialized_pub.size() << " bytes" << std::endl;
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 4 Post-Quantum Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_post_quantum();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
