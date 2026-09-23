// Kasturisundari Chain — Phase 0 Unit Tests
// Tests SHA-256, RIPEMD-160, Base58, secp256k1, and Address Generation.

#include "kasturisundari/crypto/address.h"
#include "kasturisundari/crypto/base58.h"
#include "kasturisundari/crypto/ripemd160.h"
#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include "kasturisundari/crypto/sha256.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace kasturisundari::crypto;

// ─── Color output helpers ───────────────────────────────────────────
#define GREEN "\033[32m"
#define RED   "\033[31m"
#define CYAN  "\033[36m"
#define RESET "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)                                              \
    do {                                                        \
        std::cout << CYAN "  [TEST] " RESET << name << "... ";  \
    } while (0)

#define PASS()                                                  \
    do {                                                        \
        std::cout << GREEN "PASSED" RESET << std::endl;          \
        ++tests_passed;                                         \
    } while (0)

#define FAIL(msg)                                               \
    do {                                                        \
        std::cout << RED "FAILED: " << msg << RESET << std::endl; \
        ++tests_failed;                                         \
    } while (0)

// ═══════════════════════════════════════════════════════════════════
// SHA-256 Tests
// ═══════════════════════════════════════════════════════════════════
void test_sha256() {
    std::cout << "\n=== SHA-256 Tests ===" << std::endl;

    // Test vector: SHA-256("") = e3b0c442...
    TEST("SHA-256 of empty string");
    Hash256 h = sha256(std::string(""));
    std::string hex = hash_to_hex(h);
    if (hex == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") {
        PASS();
    } else {
        FAIL("got " + hex);
    }

    // Test vector: SHA-256("abc") = ba7816bf...
    TEST("SHA-256 of 'abc'");
    h = sha256(std::string("abc"));
    hex = hash_to_hex(h);
    if (hex == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
        PASS();
    } else {
        FAIL("got " + hex);
    }

    // Test double SHA-256.
    TEST("Double SHA-256 of 'hello'");
    Hash256 dh = double_sha256(reinterpret_cast<const uint8_t*>("hello"), 5);
    hex = hash_to_hex(dh);
    if (hex.size() == 64 && hex != hash_to_hex(sha256(std::string("hello")))) {
        PASS();
    } else {
        FAIL("double hash should differ from single hash");
    }

    // Test hex round-trip.
    TEST("Hash hex round-trip");
    Hash256 original = sha256(std::string("kasturisundari"));
    std::string hex_str = hash_to_hex(original);
    Hash256 recovered = hex_to_hash(hex_str);
    if (original == recovered) {
        PASS();
    } else {
        FAIL("round-trip failed");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Base58 Tests
// ═══════════════════════════════════════════════════════════════════
void test_base58() {
    std::cout << "\n=== Base58 Tests ===" << std::endl;

    TEST("Base58 encode/decode round-trip");
    std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0xFF, 0xAB, 0xCD};
    std::string encoded = base58_encode(data);
    std::vector<uint8_t> decoded = base58_decode(encoded);
    if (data == decoded) {
        PASS();
    } else {
        FAIL("data mismatch");
    }

    TEST("Base58Check encode/decode round-trip");
    std::vector<uint8_t> payload = {0x38, 0xDE, 0xAD, 0xBE, 0xEF};
    std::string checked = base58check_encode(payload);
    std::vector<uint8_t> dec_payload = base58check_decode(checked);
    if (payload == dec_payload) {
        PASS();
    } else {
        FAIL("Base58Check round-trip failed");
    }

    TEST("Base58Check rejects corrupted data");
    std::string corrupted = checked;
    corrupted[corrupted.size() / 2] = 'X';
    std::vector<uint8_t> bad = base58check_decode(corrupted);
    if (bad.empty()) {
        PASS();
    } else {
        FAIL("should have rejected corrupted checksum");
    }
}

// ═══════════════════════════════════════════════════════════════════
// secp256k1 Tests
// ═══════════════════════════════════════════════════════════════════
void test_secp256k1() {
    std::cout << "\n=== secp256k1 Tests ===" << std::endl;

    TEST("Key generation produces valid keys");
    PrivateKey priv = generate_private_key();
    auto pub_opt = derive_public_key(priv);
    if (pub_opt.has_value()) {
        PASS();
    } else {
        FAIL("derive_public_key returned nullopt");
        return;
    }

    PublicKey pub = pub_opt.value();

    TEST("Compressed public key is 33 bytes starting with 0x02 or 0x03");
    if (pub.size() == 33 && (pub[0] == 0x02 || pub[0] == 0x03)) {
        PASS();
    } else {
        FAIL("invalid compressed public key format");
    }

    TEST("Sign and verify a message");
    Hash256 msg_hash = sha256(std::string("Kasturisundari Chain genesis"));
    auto sig_opt = sign(priv, msg_hash);
    if (!sig_opt.has_value()) {
        FAIL("signing failed");
        return;
    }
    Signature sig = sig_opt.value();
    if (verify(pub, msg_hash, sig)) {
        PASS();
    } else {
        FAIL("valid signature was rejected");
    }

    TEST("Verification fails with wrong message");
    Hash256 wrong_hash = sha256(std::string("wrong message"));
    if (!verify(pub, wrong_hash, sig)) {
        PASS();
    } else {
        FAIL("invalid message was accepted");
    }

    TEST("Verification fails with wrong public key");
    PrivateKey other_priv = generate_private_key();
    auto other_pub = derive_public_key(other_priv);
    if (!verify(other_pub.value(), msg_hash, sig)) {
        PASS();
    } else {
        FAIL("wrong public key was accepted");
    }

    TEST("Hex conversion round-trip");
    std::string priv_hex = privkey_to_hex(priv);
    std::string pub_hex = pubkey_to_hex(pub);
    std::string sig_hex = sig_to_hex(sig);
    if (priv_hex.size() == 64 && pub_hex.size() == 66 && sig_hex.size() == 128) {
        PASS();
    } else {
        FAIL("hex lengths unexpected");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Address Generation Tests
// ═══════════════════════════════════════════════════════════════════
void test_address() {
    std::cout << "\n=== Address Generation Tests ===" << std::endl;

    TEST("Generate keypair produces a valid address");
    KeyPair kp = generate_keypair();
    if (!kp.address.empty() && is_valid_address(kp.address)) {
        PASS();
    } else {
        FAIL("address is empty or invalid");
    }

    TEST("Address starts with 'K'");
    if (kp.address[0] == 'K') {
        PASS();
    } else {
        FAIL("address starts with '" + std::string(1, kp.address[0]) + "'");
    }

    TEST("Two keypairs produce different addresses");
    KeyPair kp2 = generate_keypair();
    if (kp.address != kp2.address) {
        PASS();
    } else {
        FAIL("two keypairs generated the same address");
    }

    TEST("is_valid_address rejects garbage");
    if (!is_valid_address("NotAnAddress123")) {
        PASS();
    } else {
        FAIL("garbage was accepted as valid");
    }

    TEST("is_valid_address rejects empty string");
    if (!is_valid_address("")) {
        PASS();
    } else {
        FAIL("empty string was accepted");
    }

    // Print a sample keypair for visual inspection.
    std::cout << "\n--- Sample Kasturisundari Keypair ---" << std::endl;
    std::cout << "  Private Key: " << privkey_to_hex(kp.private_key) << std::endl;
    std::cout << "  Public Key:  " << pubkey_to_hex(kp.public_key) << std::endl;
    std::cout << "  Address:     " << kp.address << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════
int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 0 Test Suite" << std::endl;
    std::cout << "============================================" << std::endl;

    test_sha256();
    test_base58();
    test_secp256k1();
    test_address();

    std::cout << "\n============================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "============================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
