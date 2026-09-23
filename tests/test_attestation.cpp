// Kasturisundari Chain — Sacred Text Attestation Tests
// Verifies immutable scripture preservation using real Vedic verses.

#include "kasturisundari/attestation/text_attestation.h"
#include "kasturisundari/crypto/sha256.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace kasturisundari::attestation;
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

// ═══════════════════════════════════════════════════════════════════
// Sample Vedic Verses for Testing
// ═══════════════════════════════════════════════════════════════════
std::vector<Verse> get_sample_rigveda_verses() {
    return {
        {1, 1, "agnim ile purohitam yajnasya devam ritvijam | hotaram ratnadhatamam ||"},
        {1, 2, "agnih purvebhir rishibhir idyo nutanair uta | sa devan eha vakshati ||"},
        {1, 3, "agnina rayim ashnavat posham eva dive-dive | yashasam viravattamam ||"},
        {1, 4, "agne yam yajnam adhvaram vishvatah paribhur asi | sa id deveshu gacchati ||"},
        {1, 5, "agnir hota kavikratuh satyash citrashravastamah | devo devebhir a gamat ||"},
    };
}

std::vector<Verse> get_sample_gita_verses() {
    return {
        {2, 47, "karmany evadhikaras te ma phaleshu kadachana | "
                "ma karma-phala-hetur bhur ma te sango 'stv akarmani ||"},
        {2, 48, "yoga-sthah kuru karmani sangam tyaktva dhananjaya | "
                "siddhy-asiddhyoh samo bhutva samatvam yoga ucyate ||"},
        {4, 7,  "yada yada hi dharmasya glanir bhavati bharata | "
                "abhyutthanam adharmasya tadatmanam srijamy aham ||"},
        {4, 8,  "paritranaya sadhunam vinashaya cha dushkritam | "
                "dharma-samsthapanarthaya sambhavami yuge yuge ||"},
    };
}

// ═══════════════════════════════════════════════════════════════════
// Attestation Tests
// ═══════════════════════════════════════════════════════════════════

void test_verse_hashing() {
    std::cout << "\n=== Verse Hashing Tests ===" << std::endl;

    TEST("Same verse always produces the same hash");
    std::string verse = "agnim ile purohitam yajnasya devam ritvijam";
    Hash256 h1 = hash_verse(verse);
    Hash256 h2 = hash_verse(verse);
    if (h1 == h2) { PASS(); } else { FAIL("hashes differ"); }

    TEST("Different verses produce different hashes");
    Hash256 h3 = hash_verse("om namo bhagavate vasudevaya");
    if (h1 != h3) { PASS(); } else { FAIL("hashes are identical"); }

    TEST("Single character change produces different hash");
    Hash256 h4 = hash_verse("agnim ile purohitam yajnasya devam ritvijam!");
    if (h1 != h4) { PASS(); } else { FAIL("hashes should differ"); }
}

void test_merkle_root() {
    std::cout << "\n=== Merkle Root Tests ===" << std::endl;

    TEST("Merkle root of single hash is itself");
    Hash256 h = sha256(std::string("test"));
    std::vector<Hash256> single = {h};
    Hash256 root = compute_merkle_root(single);
    if (root == h) { PASS(); } else { FAIL("single node root mismatch"); }

    TEST("Merkle root of empty list is zero hash");
    std::vector<Hash256> empty;
    Hash256 zero_root = compute_merkle_root(empty);
    Hash256 zeros{};
    if (zero_root == zeros) { PASS(); } else { FAIL("expected zero hash"); }

    TEST("Merkle root changes when any leaf changes");
    Hash256 h1 = sha256(std::string("verse1"));
    Hash256 h2 = sha256(std::string("verse2"));
    Hash256 h3 = sha256(std::string("verse3"));
    Hash256 root_a = compute_merkle_root({h1, h2, h3});
    Hash256 h2_mod = sha256(std::string("verse2_modified"));
    Hash256 root_b = compute_merkle_root({h1, h2_mod, h3});
    if (root_a != root_b) { PASS(); } else { FAIL("roots should differ"); }

    TEST("Merkle root is deterministic");
    Hash256 root_c = compute_merkle_root({h1, h2, h3});
    if (root_a == root_c) { PASS(); } else { FAIL("same input gave different root"); }
}

void test_scripture_attestation() {
    std::cout << "\n=== Scripture Attestation Tests ===" << std::endl;

    auto rigveda_verses = get_sample_rigveda_verses();

    TEST("Attest Rigveda Mandala 1 (5 verses)");
    ScriptureAttestation att = attest_scripture(
        "Rigveda Mandala 1 Sukta 1",
        TextCategory::VEDA,
        "Sanskrit",
        rigveda_verses);

    if (att.verses.size() == 5 && att.title == "Rigveda Mandala 1 Sukta 1") {
        PASS();
    } else {
        FAIL("attestation structure mismatch");
    }

    TEST("Category is correctly set");
    if (att.category == TextCategory::VEDA) {
        PASS();
    } else {
        FAIL("category mismatch");
    }

    TEST("Verify original verses against attestation");
    if (verify_scripture(rigveda_verses, att)) {
        PASS();
    } else {
        FAIL("original verses failed verification");
    }

    TEST("Detect tampering: modified verse content");
    auto tampered = rigveda_verses;
    tampered[2].content = "TAMPERED TEXT INSERTED HERE";
    if (!verify_scripture(tampered, att)) {
        PASS();
    } else {
        FAIL("tampered scripture was accepted!");
    }

    TEST("Detect tampering: added extra verse");
    auto extended = rigveda_verses;
    extended.push_back({1, 6, "fake verse added"});
    if (!verify_scripture(extended, att)) {
        PASS();
    } else {
        FAIL("extra verse was not detected");
    }

    TEST("Detect tampering: removed verse");
    auto shortened = rigveda_verses;
    shortened.pop_back();
    if (!verify_scripture(shortened, att)) {
        PASS();
    } else {
        FAIL("missing verse was not detected");
    }

    TEST("Detect tampering: swapped verse order");
    auto swapped = rigveda_verses;
    std::swap(swapped[0], swapped[1]);
    if (!verify_scripture(swapped, att)) {
        PASS();
    } else {
        FAIL("swapped verses were not detected");
    }

    TEST("Verify individual verse");
    bool v_ok = verify_verse(
        rigveda_verses[0].content,
        att.verses[0].content_hash);
    if (v_ok) { PASS(); } else { FAIL("single verse verification failed"); }
}

void test_gita_attestation() {
    std::cout << "\n=== Bhagavad Gita Attestation Tests ===" << std::endl;

    auto gita_verses = get_sample_gita_verses();

    TEST("Attest Bhagavad Gita selected verses");
    ScriptureAttestation att = attest_scripture(
        "Bhagavad Gita - Selected Shlokas",
        TextCategory::ITIHASA,
        "Sanskrit",
        gita_verses);

    if (att.category == TextCategory::ITIHASA && att.verses.size() == 4) {
        PASS();
    } else {
        FAIL("attestation structure mismatch");
    }

    TEST("Verify Gita verses are tamper-proof");
    if (verify_scripture(gita_verses, att)) {
        PASS();
    } else {
        FAIL("verification failed");
    }

    TEST("Detect single character change in Chapter 2 Verse 47");
    auto tampered = gita_verses;
    // Change 'karmany' to 'Karmany' — single character case change.
    tampered[0].content[0] = 'K';
    if (!verify_scripture(tampered, att)) {
        PASS();
    } else {
        FAIL("single char change was not detected!");
    }
}

void test_serialization() {
    std::cout << "\n=== Serialization Tests ===" << std::endl;

    auto verses = get_sample_rigveda_verses();
    ScriptureAttestation original = attest_scripture(
        "Rigveda Test", TextCategory::VEDA, "Sanskrit", verses);

    TEST("Serialize and deserialize attestation");
    std::vector<uint8_t> binary = serialize_attestation(original);
    ScriptureAttestation recovered = deserialize_attestation(binary);

    bool match = (recovered.title == original.title
               && recovered.category == original.category
               && recovered.language == original.language
               && recovered.verses.size() == original.verses.size()
               && recovered.merkle_root == original.merkle_root
               && recovered.full_text_hash == original.full_text_hash);
    if (match) { PASS(); } else { FAIL("deserialized data mismatch"); }

    TEST("Deserialized attestation still verifies original verses");
    if (verify_scripture(verses, recovered)) {
        PASS();
    } else {
        FAIL("verification failed after deserialization");
    }

    TEST("Category string conversion");
    if (std::string(category_to_string(TextCategory::VEDA)) == "Veda"
        && std::string(category_to_string(TextCategory::ITIHASA)) == "Itihasa"
        && std::string(category_to_string(TextCategory::STOTRA)) == "Stotra") {
        PASS();
    } else {
        FAIL("category names mismatch");
    }
}

void test_display_attestation() {
    auto verses = get_sample_rigveda_verses();
    ScriptureAttestation att = attest_scripture(
        "Rigveda Mandala 1 Sukta 1", TextCategory::VEDA, "Sanskrit", verses);

    std::cout << "\n--- Sample Scripture Attestation ---" << std::endl;
    std::cout << "  Title:       " << att.title << std::endl;
    std::cout << "  Category:    " << category_to_string(att.category) << std::endl;
    std::cout << "  Language:    " << att.language << std::endl;
    std::cout << "  Verses:      " << att.verses.size() << std::endl;
    std::cout << "  Merkle Root: " << hash_to_hex(att.merkle_root) << std::endl;
    std::cout << "  Full Hash:   " << hash_to_hex(att.full_text_hash) << std::endl;
    std::cout << "  Verse Hashes:" << std::endl;
    for (size_t i = 0; i < att.verses.size(); ++i) {
        std::cout << "    [" << att.verses[i].chapter << ":"
                  << att.verses[i].verse_num << "] "
                  << hash_to_hex(att.verses[i].content_hash) << std::endl;
    }
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Text Attestation Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_verse_hashing();
    test_merkle_root();
    test_scripture_attestation();
    test_gita_attestation();
    test_serialization();
    test_display_attestation();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
