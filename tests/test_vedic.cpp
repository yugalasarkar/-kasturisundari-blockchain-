// Kasturisundari Chain — Phase 7 Vedic Preservation Tests
// Tests the Panini Grammar Engine and Audio Shabda Attestation.

#include "kasturisundari/attestation/panini_engine.h"
#include "kasturisundari/attestation/audio_attestation.h"
#include "kasturisundari/crypto/sha256.h"

#include <iostream>
#include <string>
#include <vector>

using namespace kasturisundari;
using namespace kasturisundari::vedic;

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

void test_vedic() {
    std::cout << "\n=== Vedic Preservation (Phase 7) Tests ===" << std::endl;

    // --- 1. Panini Grammar Engine Tests ---
    std::cout << "\n--- Panini Grammar Engine ---" << std::endl;

    TEST("Accept valid Sanskrit Devanagari text");
    // "ॐ भूर्भुवः स्वः" (Om bhur bhuvah svah)
    std::string valid_sanskrit = "ॐ भूर्भुवः स्वः";
    auto res = PaniniEngine::validate_sanskrit_text(valid_sanskrit);
    if (res.is_valid) {
        PASS();
    } else {
        FAIL(res.error_message);
    }

    TEST("Reject text with English alphabet");
    std::string mixed_text = "ॐ भूर्भुवः svah";
    res = PaniniEngine::validate_sanskrit_text(mixed_text);
    if (!res.is_valid) {
        PASS();
    } else {
        FAIL("accepted non-Devanagari text");
    }

    TEST("Reject text with Chinese characters");
    std::string chinese_text = "ॐ भूर्भुवः 汉字";
    res = PaniniEngine::validate_sanskrit_text(chinese_text);
    if (!res.is_valid) {
        PASS();
    } else {
        FAIL("accepted Chinese characters");
    }

    TEST("Identify Svara (Vowels)");
    if (PaniniEngine::is_svara(0x0905) && PaniniEngine::is_svara(0x093E)) { // 'अ' and 'ा'
        PASS();
    } else {
        FAIL("failed to identify vowels");
    }

    TEST("Identify Vyanjana (Consonants)");
    if (PaniniEngine::is_vyanjana(0x0915) && PaniniEngine::is_vyanjana(0x092A)) { // 'क' and 'प'
        PASS();
    } else {
        FAIL("failed to identify consonants");
    }

    // --- 2. Audio Shabda Attestation Tests ---
    std::cout << "\n--- Audio Shabda Attestation ---" << std::endl;

    TEST("Create Audio Attestation (Mock PCM Data)");
    std::vector<uint8_t> mock_audio;
    for (int i = 0; i < 20000; ++i) {
        mock_audio.push_back(i % 256); // generate 20KB of fake PCM data
    }

    auto attestation = ShabdaEngine::create_attestation(
        "Gayatri Mantra", "Vedic Scholar", 
        44100, 16, 2, mock_audio, 4096 // 4KB chunks
    );

    if (attestation.title == "Gayatri Mantra" && attestation.merkle_root != crypto::Hash256{0}) {
        PASS();
    } else {
        FAIL("failed to create audio attestation");
    }

    TEST("Verify exact matching audio");
    if (ShabdaEngine::verify_audio(attestation, mock_audio, 4096)) {
        PASS();
    } else {
        FAIL("valid audio rejected");
    }

    TEST("Detect tampered audio (1 byte changed)");
    std::vector<uint8_t> tampered_audio = mock_audio;
    tampered_audio[10500] = 0xFF; // Change a frequency byte
    if (!ShabdaEngine::verify_audio(attestation, tampered_audio, 4096)) {
        PASS();
    } else {
        FAIL("accepted tampered audio");
    }

    TEST("Serialize and Deserialize Audio Attestation");
    auto serialized = attestation.serialize();
    auto deserialized = AudioAttestation::deserialize(serialized);
    if (deserialized.title == attestation.title && 
        deserialized.merkle_root == attestation.merkle_root && 
        deserialized.full_audio_hash == attestation.full_audio_hash) {
        PASS();
    } else {
        FAIL("serialization mismatch");
    }
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 7 Vedic Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_vedic();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
