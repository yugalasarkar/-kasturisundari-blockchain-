// Kasturisundari Chain — Phase 2 Tests
// Tests StateDB, Tokenomics, and Genesis Block.

#include "kasturisundari/economics/tokenomics.h"
#include "kasturisundari/genesis/genesis.h"
#include "kasturisundari/state/state_db.h"
#include "kasturisundari/crypto/address.h"
#include "kasturisundari/crypto/sha256.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

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

// ═══════════════════════════════════════════════════════════════════
// Tokenomics Tests
// ═══════════════════════════════════════════════════════════════════
void test_tokenomics() {
    std::cout << "\n=== Tokenomics Tests ===" << std::endl;
    using namespace economics;

    TEST("MAX_SUPPLY equals 21 million NSK");
    if (MAX_SUPPLY == 21'000'000ULL * UNITS_PER_NILA) {
        PASS();
    } else {
        FAIL("MAX_SUPPLY mismatch");
    }

    TEST("Allocations sum to MAX_SUPPLY");
    uint64_t total = FOUNDER_ALLOCATION + SECONDARY_ALLOCATION + INITIAL_REWARD_POOL;
    if (total == MAX_SUPPLY) {
        PASS();
    } else {
        FAIL("allocations don't sum to 21M");
    }

    TEST("would_exceed_cap detects overflow");
    if (would_exceed_cap(MAX_SUPPLY, 1)) {
        PASS();
    } else {
        FAIL("should detect cap exceeded");
    }

    TEST("would_exceed_cap allows valid amount");
    if (!would_exceed_cap(MAX_SUPPLY - 1000, 1000)) {
        PASS();
    } else {
        FAIL("should allow exact cap");
    }

    TEST("Contribution reward at full pool");
    uint64_t reward = compute_contribution_reward(INITIAL_BASE_REWARD, INITIAL_REWARD_POOL);
    if (reward == INITIAL_BASE_REWARD) {
        PASS();
    } else {
        FAIL("expected full base reward, got " + format_nila(reward));
    }

    TEST("Contribution reward at half pool");
    reward = compute_contribution_reward(INITIAL_BASE_REWARD, INITIAL_REWARD_POOL / 2);
    uint64_t expected = INITIAL_BASE_REWARD / 2;
    if (reward == expected) {
        PASS();
    } else {
        FAIL("expected " + format_nila(expected) + ", got " + format_nila(reward));
    }

    TEST("Contribution reward at 1% pool");
    reward = compute_contribution_reward(INITIAL_BASE_REWARD, INITIAL_REWARD_POOL / 100);
    expected = INITIAL_BASE_REWARD / 100;
    if (reward == expected) {
        PASS();
    } else {
        FAIL("expected " + format_nila(expected) + ", got " + format_nila(reward));
    }

    TEST("Contribution reward at empty pool is 0");
    reward = compute_contribution_reward(INITIAL_BASE_REWARD, 0);
    if (reward == 0) {
        PASS();
    } else {
        FAIL("expected 0");
    }

    TEST("Era base reward halving");
    uint64_t era0 = compute_era_base_reward(INITIAL_BASE_REWARD, 0, ERA_BLOCK_INTERVAL);
    uint64_t era1 = compute_era_base_reward(INITIAL_BASE_REWARD, ERA_BLOCK_INTERVAL, ERA_BLOCK_INTERVAL);
    uint64_t era2 = compute_era_base_reward(INITIAL_BASE_REWARD, ERA_BLOCK_INTERVAL * 2, ERA_BLOCK_INTERVAL);
    if (era0 == INITIAL_BASE_REWARD && era1 == INITIAL_BASE_REWARD / 2 && era2 == INITIAL_BASE_REWARD / 4) {
        PASS();
    } else {
        FAIL("halving incorrect");
    }

    TEST("Supply invariant validation");
    if (validate_supply_invariant(FOUNDER_ALLOCATION, SECONDARY_ALLOCATION, INITIAL_REWARD_POOL, 0)) {
        PASS();
    } else {
        FAIL("invariant should hold at genesis");
    }

    TEST("Supply invariant detects imbalance");
    if (!validate_supply_invariant(FOUNDER_ALLOCATION, SECONDARY_ALLOCATION, INITIAL_REWARD_POOL, 1)) {
        PASS();
    } else {
        FAIL("should detect extra unit");
    }

    TEST("format_nila works correctly");
    std::string s = format_nila(150'000'000ULL);  // 1.5 NSK
    if (s == "1.50000000") {
        PASS();
    } else {
        FAIL("got '" + s + "'");
    }

    TEST("parse_nsk works correctly");
    uint64_t parsed = parse_nsk("1.5");
    if (parsed == 150'000'000ULL) {
        PASS();
    } else {
        FAIL("got " + std::to_string(parsed));
    }

    TEST("format/parse round-trip");
    uint64_t original = 123'456'789'00ULL;
    std::string formatted = format_nila(original);
    uint64_t recovered = parse_nsk(formatted);
    if (recovered == original) {
        PASS();
    } else {
        FAIL("round-trip failed: " + formatted);
    }

    // Display reward curve.
    std::cout << "\n--- Reward Diminishing Curve ---" << std::endl;
    for (int pct : {100, 75, 50, 25, 10, 5, 1}) {
        uint64_t pool = INITIAL_REWARD_POOL * pct / 100;
        uint64_t r = compute_contribution_reward(INITIAL_BASE_REWARD, pool);
        std::cout << "  Pool at " << pct << "%: reward = "
                  << format_nila(r) << " NSK" << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════
// StateDB Tests
// ═══════════════════════════════════════════════════════════════════
void test_state_db() {
    std::cout << "\n=== StateDB Tests ===" << std::endl;

    std::string db_path = "./test_kasturisundari_db";

    // Clean up any previous test DB.
    std::filesystem::remove_all(db_path);

    state::StateDB db;

    TEST("Open database");
    if (db.open(db_path)) {
        PASS();
    } else {
        FAIL("could not open DB");
        return;
    }

    TEST("Database is open");
    if (db.is_open()) {
        PASS();
    } else {
        FAIL("db reports not open");
    }

    TEST("Non-existent account returns zero balance");
    auto acc = db.get_account("KNonExistent");
    if (acc.balance == 0 && acc.nonce == 0) {
        PASS();
    } else {
        FAIL("expected zero account");
    }

    TEST("Put and get account");
    state::Account alice{1'000'000'00000000ULL, 5};
    db.put_account("KAliceAddress", alice);
    auto retrieved = db.get_account("KAliceAddress");
    if (retrieved.balance == alice.balance && retrieved.nonce == alice.nonce) {
        PASS();
    } else {
        FAIL("account data mismatch");
    }

    TEST("has_account returns true for existing");
    if (db.has_account("KAliceAddress")) {
        PASS();
    } else {
        FAIL("should exist");
    }

    TEST("has_account returns false for non-existing");
    if (!db.has_account("KNonExistent")) {
        PASS();
    } else {
        FAIL("should not exist");
    }

    TEST("get_balance shortcut");
    if (db.get_balance("KAliceAddress") == alice.balance) {
        PASS();
    } else {
        FAIL("balance mismatch");
    }

    TEST("Store and retrieve metadata");
    db.put_meta("chain_name", "Kasturisundari");
    auto val = db.get_meta("chain_name");
    if (val && *val == "Kasturisundari") {
        PASS();
    } else {
        FAIL("metadata mismatch");
    }

    TEST("Store and retrieve chain height");
    db.put_chain_height(42);
    auto h = db.get_chain_height();
    if (h && *h == 42) {
        PASS();
    } else {
        FAIL("height mismatch");
    }

    TEST("Store and retrieve latest block hash");
    auto hash = crypto::sha256(std::string("test_block"));
    db.put_latest_block_hash(hash);
    auto retrieved_hash = db.get_latest_block_hash();
    if (retrieved_hash && *retrieved_hash == hash) {
        PASS();
    } else {
        FAIL("block hash mismatch");
    }

    db.close();

    TEST("Database is closed");
    if (!db.is_open()) {
        PASS();
    } else {
        FAIL("db reports still open");
    }

    // Clean up.
    std::filesystem::remove_all(db_path);
}

// ═══════════════════════════════════════════════════════════════════
// Genesis Tests
// ═══════════════════════════════════════════════════════════════════
void test_genesis() {
    std::cout << "\n=== Genesis Block Tests ===" << std::endl;
    using namespace economics;

    crypto::KeyPair founder   = crypto::generate_keypair();
    crypto::KeyPair secondary = crypto::generate_keypair();

    genesis::GenesisConfig config;
    config.founder_address     = founder.address;
    config.secondary_address   = secondary.address;
    config.reward_pool_address = REWARD_POOL_ADDRESS;
    config.timestamp           = 1720000000;  // Fixed timestamp for reproducibility.

    TEST("Build genesis block");
    core::Block gen = genesis::build_genesis_block(config);
    if (gen.header.height == 0 && gen.transactions.size() == 3) {
        PASS();
    } else {
        FAIL("genesis structure wrong");
    }

    TEST("Genesis is valid");
    if (genesis::is_valid_genesis(gen)) {
        PASS();
    } else {
        FAIL("genesis validation failed");
    }

    TEST("Genesis allocates exactly 21,000,000 NSK");
    uint64_t total = 0;
    for (const auto& tx : gen.transactions) total += tx.amount;
    if (total == MAX_SUPPLY) {
        PASS();
    } else {
        FAIL("total = " + format_nila(total));
    }

    TEST("Genesis has zero fees");
    if (gen.header.total_fees == 0) {
        PASS();
    } else {
        FAIL("fees should be 0");
    }

    TEST("Genesis validates internally");
    if (gen.validate_internal()) {
        PASS();
    } else {
        FAIL("internal validation failed");
    }

    // Apply to state DB.
    std::string db_path = "./test_genesis_db";
    std::filesystem::remove_all(db_path);

    state::StateDB db;
    db.open(db_path);

    TEST("Apply genesis to state DB");
    if (genesis::apply_genesis_to_state(gen, db)) {
        PASS();
    } else {
        FAIL("apply failed");
    }

    TEST("Founder has 1,000,000 NSK");
    uint64_t fb = db.get_balance(founder.address);
    if (fb == FOUNDER_ALLOCATION) {
        PASS();
    } else {
        FAIL("founder balance = " + format_nila(fb));
    }

    TEST("Secondary has 1,000,000 NSK");
    uint64_t sb = db.get_balance(secondary.address);
    if (sb == SECONDARY_ALLOCATION) {
        PASS();
    } else {
        FAIL("secondary balance = " + format_nila(sb));
    }

    TEST("Reward Pool has 19,000,000 NSK");
    uint64_t rb = db.get_balance(REWARD_POOL_ADDRESS);
    if (rb == INITIAL_REWARD_POOL) {
        PASS();
    } else {
        FAIL("pool balance = " + format_nila(rb));
    }

    TEST("Supply invariant holds after genesis");
    if (validate_supply_invariant(fb, sb, rb, 0)) {
        PASS();
    } else {
        FAIL("invariant broken!");
    }

    TEST("Chain height is 0");
    auto h = db.get_chain_height();
    if (h && *h == 0) {
        PASS();
    } else {
        FAIL("height wrong");
    }

    TEST("Latest block hash matches genesis");
    auto stored_hash = db.get_latest_block_hash();
    if (stored_hash && *stored_hash == gen.compute_hash()) {
        PASS();
    } else {
        FAIL("hash mismatch");
    }

    db.close();
    std::filesystem::remove_all(db_path);

    // Display genesis info.
    std::cout << "\n--- Genesis Block Summary ---" << std::endl;
    std::cout << "  Block Hash:    " << gen.block_id().substr(0, 32) << "..." << std::endl;
    std::cout << "  Transactions:  " << gen.transactions.size() << std::endl;
    std::cout << "  Founder:       " << founder.address << " -> "
              << format_nila(FOUNDER_ALLOCATION) << " NSK" << std::endl;
    std::cout << "  Secondary:     " << secondary.address << " -> "
              << format_nila(SECONDARY_ALLOCATION) << " NSK" << std::endl;
    std::cout << "  Reward Pool:   " << REWARD_POOL_ADDRESS << " -> "
              << format_nila(INITIAL_REWARD_POOL) << " NSK" << std::endl;
    std::cout << "  Total Supply:  " << format_nila(MAX_SUPPLY) << " NSK" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 2 State Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_tokenomics();
    test_state_db();
    test_genesis();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
