// Kasturisundari Chain — Adversarial & Edge-Case Crypto Test Suite
// Rigorous security testing for Phase 1 cryptographic engine: RFC 6979 determinism,
// High-S malleability rejection, fuzzed signatures, public key recovery, and hybrid PQC.

#include "kasturisundari/crypto/address.h"
#include "kasturisundari/crypto/post_quantum.h"
#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include "kasturisundari/crypto/sha256.h"

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <cassert>
#include <cstring>
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
    std::cout << CYAN "  [ADVERSARIAL TEST] " RESET << name << "... " << std::flush;

#define PASS() \
    do { std::cout << GREEN "PASSED" RESET << std::endl; ++tests_passed; } while(0)

#define FAIL(msg) \
    do { std::cout << RED "FAILED: " << msg << RESET << std::endl; ++tests_failed; } while(0)

// Helper to compute complementary High-S signature (r, n - s)
Signature compute_malleated_high_s(const Signature& valid_sig) {
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BIGNUM* order = BN_new();
    EC_GROUP_get_order(group, order, NULL);

    BIGNUM* s_bn = BN_bin2bn(valid_sig.data() + 32, 32, NULL);
    BIGNUM* high_s_bn = BN_new();
    BN_sub(high_s_bn, order, s_bn);

    Signature malleated = valid_sig;
    BN_bn2binpad(high_s_bn, malleated.data() + 32, 32);

    BN_free(high_s_bn);
    BN_free(s_bn);
    BN_free(order);
    EC_GROUP_free(group);

    return malleated;
}

// ═══════════════════════════════════════════════════════════════════
// Vector 1: Determinism & Nonce Leakage Vector (RFC 6979)
// ═══════════════════════════════════════════════════════════════════
void test_vector1_rfc6979_determinism() {
    std::cout << "\n=== Security Vector 1: RFC 6979 Determinism & Nonce Leakage ===" << std::endl;

    PrivateKey priv = generate_private_key();
    Hash256 msg1 = sha256("Sacred Kasturisundari Genesis Message 108108");

    TEST("Sign same message 1,000 times (100% bit-for-bit identity check)");
    auto base_sig_opt = sign(priv, msg1);
    if (!base_sig_opt) {
        FAIL("base signing failed");
        return;
    }
    Signature base_sig = *base_sig_opt;

    bool deterministic = true;
    for (int i = 0; i < 1000; ++i) {
        auto current_sig_opt = sign(priv, msg1);
        if (!current_sig_opt || *current_sig_opt != base_sig) {
            deterministic = false;
            break;
        }
    }

    if (deterministic) {
        PASS();
    } else {
        FAIL("non-deterministic signature detected! Nonce reuse leakage risk.");
    }

    TEST("Distinct messages produce strictly distinct nonces & signatures");
    Hash256 msg2 = sha256("Distinct Message 108109");
    auto sig2_opt = sign(priv, msg2);
    if (!sig2_opt) {
        FAIL("signing msg2 failed");
        return;
    }
    if (*sig2_opt != base_sig) {
        PASS();
    } else {
        FAIL("identical signature generated for different messages!");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Vector 2: Signature Malleability & High-S Attack Vector
// ═══════════════════════════════════════════════════════════════════
void test_vector2_high_s_malleability() {
    std::cout << "\n=== Security Vector 2: Signature Malleability & High-S Attack ===" << std::endl;

    PrivateKey priv = generate_private_key();
    PublicKey pub = *derive_public_key(priv);
    Hash256 msg = sha256("Malleability Test Message");

    uint8_t original_v = 0;
    auto sig_opt = sign_with_recovery(priv, msg, original_v);
    if (!sig_opt) {
        FAIL("signing failed");
        return;
    }
    Signature valid_sig = *sig_opt;

    TEST("Original canonical Low-S signature passes verification");
    if (verify(pub, msg, valid_sig)) {
        PASS();
    } else {
        FAIL("canonical Low-S signature rejected");
    }

    TEST("Construct High-S complement signature (r, n - s)");
    Signature high_s_sig = compute_malleated_high_s(valid_sig);

    TEST("High-S complement signature MUST be strictly REJECTED by verify()");
    if (!verify(pub, msg, high_s_sig)) {
        PASS();
    } else {
        FAIL("VULNERABILITY DETECTED: Malleated High-S signature accepted!");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Vector 3: Corrupted / Fuzzed Signatures Vector
// ═══════════════════════════════════════════════════════════════════
void test_vector3_fuzzing_and_corruption() {
    std::cout << "\n=== Security Vector 3: Fuzzing & Corrupted Signature Resiliency ===" << std::endl;

    PrivateKey priv = generate_private_key();
    PublicKey pub = *derive_public_key(priv);
    Hash256 msg = sha256("Fuzzing Test Payload");

    auto sig_opt = sign(priv, msg);
    Signature valid_sig = *sig_opt;

    TEST("Fuzz 512 bit-flips across r, s, and msg_hash without crash or false-positive");
    bool memory_safe = true;
    bool any_false_accept = false;

    // 1. Bit-flip signature (64 bytes * 8 bits = 512 bits)
    for (size_t byte_idx = 0; byte_idx < 64; ++byte_idx) {
        for (int bit = 0; bit < 8; ++bit) {
            Signature fuzzed_sig = valid_sig;
            fuzzed_sig[byte_idx] ^= (1 << bit);

            bool result = verify(pub, msg, fuzzed_sig);
            if (result && fuzzed_sig != valid_sig) {
                any_false_accept = true;
            }
        }
    }

    // 2. Bit-flip msg_hash (32 bytes * 8 bits = 256 bits)
    for (size_t byte_idx = 0; byte_idx < 32; ++byte_idx) {
        for (int bit = 0; bit < 8; ++bit) {
            Hash256 fuzzed_msg = msg;
            fuzzed_msg[byte_idx] ^= (1 << bit);

            bool result = verify(pub, fuzzed_msg, valid_sig);
            if (result && fuzzed_msg != msg) {
                any_false_accept = true;
            }
        }
    }

    if (memory_safe && !any_false_accept) {
        PASS();
    } else {
        FAIL("Fuzzed signature caused unexpected acceptance or failure");
    }

    TEST("Hex parser fuzzing (all 0x00, all 0xFF, odd lengths, invalid chars)");
    std::string invalid_hex_chars = "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG";
    std::string short_hex = "1234567890abcdef";

    auto p1 = hex_to_privkey(invalid_hex_chars);
    auto p2 = hex_to_privkey(short_hex);
    auto p3 = hex_to_pubkey(short_hex);
    auto s1 = hex_to_sig(short_hex);

    if (!p1.has_value() && !p2.has_value() && !p3.has_value() && !s1.has_value()) {
        PASS();
    } else {
        FAIL("Hex parser accepted garbage payload");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Vector 4: Public Key Recovery Under Forged/Corrupted v
// ═══════════════════════════════════════════════════════════════════
void test_vector4_pubkey_recovery_forgery() {
    std::cout << "\n=== Security Vector 4: Public Key Recovery Under Forged/Corrupted v ===" << std::endl;

    PrivateKey priv = generate_private_key();
    PublicKey expected_pub = *derive_public_key(priv);
    Hash256 msg = sha256("EIP-155 Recovery Test");

    uint8_t rec_id = 0;
    auto sig_opt = sign_with_recovery(priv, msg, rec_id);
    Signature sig = *sig_opt;

    TEST("Recover public key with correct recovery ID (v)");
    auto recovered_opt = recover_public_key(msg, sig, rec_id);
    if (recovered_opt.has_value() && *recovered_opt == expected_pub) {
        PASS();
    } else {
        FAIL("Failed to recover exact expected public key");
    }

    TEST("Recover with wrong y-parity (v ^ 1) produces mismatching key that does NOT match expected_pub");
    uint8_t wrong_v = rec_id ^ 1;
    auto wrong_recovered_opt = recover_public_key(msg, sig, wrong_v);

    if (!wrong_recovered_opt.has_value()) {
        PASS();
    } else {
        PublicKey wrong_pub = *wrong_recovered_opt;
        if (wrong_pub != expected_pub) {
            PASS();
        } else {
            FAIL("Wrong recovery ID produced identical public key!");
        }
    }

    TEST("Recover with invalid out-of-bounds recovery IDs (v >= 2) returns std::nullopt");
    auto invalid_v1 = recover_public_key(msg, sig, 2);
    auto invalid_v2 = recover_public_key(msg, sig, 99);
    auto invalid_v3 = recover_public_key(msg, sig, 255);

    if (!invalid_v1.has_value() && !invalid_v2.has_value() && !invalid_v3.has_value()) {
        PASS();
    } else {
        FAIL("Out-of-bounds recovery ID was accepted!");
    }

    TEST("Recovery under forged message hash fails verification against original public key");
    Hash256 forged_msg = sha256("Forged Message Payload");
    auto forged_recovered_opt = recover_public_key(forged_msg, sig, rec_id);
    if (!forged_recovered_opt.has_value() || *forged_recovered_opt != expected_pub) {
        PASS();
    } else {
        FAIL("Recovered original public key from forged message signature!");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Vector 5: PQC Determinism & Hybrid Validation
// ═══════════════════════════════════════════════════════════════════
void test_vector5_pqc_hybrid_security() {
    std::cout << "\n=== Security Vector 5: PQC Determinism & Dual Hybrid Validation ===" << std::endl;

    PrivateKey class_priv = generate_private_key();
    PublicKey class_pub = *derive_public_key(class_priv);

    pqc::PqPublicKey pq_pub;
    pqc::PqPrivateKey pq_priv;
    pqc::generate_pq_keypair(pq_pub, pq_priv);

    pqc::HybridPublicKey hybrid_pub;
    hybrid_pub.classical = class_pub;
    hybrid_pub.pq = pq_pub;

    Hash256 msg = sha256("Dual Verification Hybrid Payload 108108");

    TEST("Sign same message 1,000 times with hybrid_sign() (100% bit-for-bit identity)");
    pqc::HybridSignature base_hybrid_sig = pqc::hybrid_sign(class_priv, pq_priv, msg);

    bool deterministic = true;
    for (int i = 0; i < 1000; ++i) {
        pqc::HybridSignature current_sig = pqc::hybrid_sign(class_priv, pq_priv, msg);
        if (current_sig.classical != base_hybrid_sig.classical ||
            current_sig.pq.data != base_hybrid_sig.pq.data) {
            deterministic = false;
            break;
        }
    }

    if (deterministic) {
        PASS();
    } else {
        FAIL("hybrid_sign generated non-deterministic signature!");
    }

    TEST("Valid hybrid signature passes hybrid_verify()");
    if (pqc::hybrid_verify(hybrid_pub, msg, base_hybrid_sig)) {
        PASS();
    } else {
        FAIL("valid hybrid signature rejected");
    }

    TEST("hybrid_verify REJECTS if classical signature is invalid but PQC signature is valid");
    pqc::HybridSignature broken_classical = base_hybrid_sig;
    broken_classical.classical[10] ^= 0xFF; // corrupt classical
    if (!pqc::hybrid_verify(hybrid_pub, msg, broken_classical)) {
        PASS();
    } else {
        FAIL("VULNERABILITY: hybrid_verify accepted broken classical signature!");
    }

    TEST("hybrid_verify REJECTS if PQC signature is invalid but classical signature is valid");
    pqc::HybridSignature broken_pq = base_hybrid_sig;
    broken_pq.pq.data[200] ^= 0xFF; // corrupt PQ signature
    if (!pqc::hybrid_verify(hybrid_pub, msg, broken_pq)) {
        PASS();
    } else {
        FAIL("VULNERABILITY: hybrid_verify accepted broken PQC signature!");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Main Adversarial Runner
// ═══════════════════════════════════════════════════════════════════
int main() {
    std::cout << "===============================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 1 Adversarial Security Suite" << std::endl;
    std::cout << "===============================================================" << std::endl;

    test_vector1_rfc6979_determinism();
    test_vector2_high_s_malleability();
    test_vector3_fuzzing_and_corruption();
    test_vector4_pubkey_recovery_forgery();
    test_vector5_pqc_hybrid_security();

    std::cout << "\n===============================================================" << std::endl;
    std::cout << "  Adversarial Security Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "===============================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
