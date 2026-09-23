// Kasturisundari Chain — Phase 10 Tests
// Tests Vedic Storage (P2P File System) and Vedic Sabha (Governance).

#include "kasturisundari/storage/vedic_storage.h"
#include "kasturisundari/governance/sabha.h"
#include "kasturisundari/state/state_db.h"
#include "kasturisundari/crypto/sha256.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

using namespace kasturisundari;

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

void test_phase10() {
    std::cout << "\n=== Phase 10: Advanced Decentralization Tests ===" << std::endl;

    // --- 1. Vedic Storage Tests ---
    std::cout << "\n--- Vedic Storage (P2P File Engine) ---" << std::endl;
    storage::VedicStorage storage("./test_storage_data");
    
    TEST("Initialize storage directory");
    if (storage.init()) {
        PASS();
    } else {
        FAIL("Failed to create storage directory");
    }

    TEST("Store a file chunk");
    std::vector<uint8_t> mock_file = {'O', 'M', ' ', 'S', 'H', 'A', 'N', 'T', 'I'};
    crypto::Hash256 chunk_hash = crypto::sha256(mock_file.data(), mock_file.size());
    if (storage.store_chunk(mock_file)) {
        PASS();
    } else {
        FAIL("Failed to store file chunk");
    }

    TEST("Check if chunk exists");
    if (storage.has_chunk(chunk_hash)) {
        PASS();
    } else {
        FAIL("Chunk not found after storage");
    }

    TEST("Retrieve and verify file chunk");
    auto retrieved = storage.get_chunk(chunk_hash);
    if (retrieved == mock_file) {
        PASS();
    } else {
        FAIL("Retrieved chunk data mismatch");
    }

    // --- 2. Vedic Sabha Tests ---
    std::cout << "\n--- Vedic Sabhā (Governance Voting) ---" << std::endl;
    state::StateDB state_db;
    state_db.open("./test_sabha_db");
    governance::Sabha sabha(state_db);

    // Mock an account balance for voting
    state::Account acc;
    acc.balance = 5000; // 5000 NSK voting weight
    acc.nonce = 0;
    state_db.put_account("KVoterAddress", acc);

    TEST("Submit Governance Proposal");
    crypto::Hash256 proposal_hash = crypto::sha256((const uint8_t*)"UPGRADE_PROTOCOL", 16);
    if (sabha.submit_proposal(proposal_hash, "KProposer")) {
        PASS();
    } else {
        FAIL("Failed to submit proposal");
    }

    TEST("Cast Vote (Approval)");
    if (sabha.cast_vote(proposal_hash, "KVoterAddress", true)) {
        PASS();
    } else {
        FAIL("Failed to cast vote");
    }

    TEST("Prevent Double Voting");
    if (!sabha.cast_vote(proposal_hash, "KVoterAddress", true)) {
        PASS();
    } else {
        FAIL("Allowed double voting");
    }

    TEST("Verify Voting Tally (NSK Weighted)");
    auto tally = sabha.get_tally(proposal_hash);
    if (tally.first == 5000 && tally.second == 0) {
        PASS();
    } else {
        FAIL("Incorrect tally weight");
    }

    // Cleanup storage dir and DB
    std::filesystem::remove_all("./test_storage_data");
    std::filesystem::remove("./test_sabha_db");
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 10 Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_phase10();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
