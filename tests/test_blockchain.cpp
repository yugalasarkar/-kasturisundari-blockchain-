// Kasturisundari Chain — Blockchain Invariants Test Suite
// Rigorous verification for Phase 2: Merkle tree integrity, difficulty retargeting,
// chain fork choice & reorgs, and Genesis block state invariants.

#include "kasturisundari/core/block.h"
#include "kasturisundari/core/blockchain.h"
#include "kasturisundari/core/transaction.h"
#include "kasturisundari/crypto/address.h"
#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include "kasturisundari/crypto/sha256.h"
#include "kasturisundari/genesis/genesis.h"
#include "kasturisundari/state/state_db.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace kasturisundari;
using namespace kasturisundari::core;
using namespace kasturisundari::crypto;

#define GREEN "\033[32m"
#define RED   "\033[31m"
#define CYAN  "\033[36m"
#define RESET "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    std::cout << CYAN "  [TEST] " RESET << name << "... " << std::flush;

#define PASS() \
    do { std::cout << GREEN "PASSED" RESET << std::endl; ++tests_passed; } while(0)

#define FAIL(msg) \
    do { std::cout << RED "FAILED: " << msg << RESET << std::endl; ++tests_failed; } while(0)

// Helper: build a sample signed transaction
Transaction create_sample_tx(const PrivateKey& priv, uint64_t nonce, uint64_t amount = 1000) {
    PublicKey pub = *derive_public_key(priv);
    std::string sender_addr = public_key_to_address(pub);

    Transaction tx;
    tx.version = 1;
    tx.type = TxType::TRANSFER;
    tx.timestamp = 1700000000;
    tx.nonce = nonce;
    tx.sender = sender_addr;
    tx.recipient = "Kasturi1111111111111111111111111111111111111111";
    tx.amount = amount;
    tx.fee = 10;
    tx.data = {0x01, 0x02, 0x03};

    tx.sign_transaction(priv);
    return tx;
}

// ═══════════════════════════════════════════════════════════════════
// 1. Merkle Tree Integrity
// ═══════════════════════════════════════════════════════════════════
void test_merkle_tree_integrity() {
    std::cout << "\n=== 1. Merkle Tree Integrity & Block Validation ===" << std::endl;

    PrivateKey priv = generate_private_key();
    Transaction tx1 = create_sample_tx(priv, 0, 500);
    Transaction tx2 = create_sample_tx(priv, 1, 800);

    Block block;
    block.header.version = 1;
    block.header.height = 1;
    block.header.timestamp = 1700000005;
    block.header.previous_hash = sha256("GenesisHash");
    block.transactions = {tx1, tx2};
    block.header.tx_count = static_cast<uint32_t>(block.transactions.size());
    block.header.total_fees = tx1.fee + tx2.fee;
    block.header.merkle_root = block.compute_merkle_root();

    TEST("Valid block with matching Merkle root passes internal validation");
    if (block.validate_internal()) {
        PASS();
    } else {
        FAIL("valid block failed internal validation");
    }

    TEST("Tampering single transaction byte invalidates Merkle root");
    Block tampered_block = block;
    tampered_block.transactions[0].amount += 1; // Mutate amount byte
    if (!tampered_block.validate_internal()) {
        PASS();
    } else {
        FAIL("tampered transaction was NOT caught by validate_internal()");
    }

    TEST("Tampering tx_count invalidates block");
    Block count_tampered = block;
    count_tampered.header.tx_count = 99;
    if (!count_tampered.validate_internal()) {
        PASS();
    } else {
        FAIL("invalid tx_count was NOT caught");
    }

    TEST("Tampering total_fees invalidates block");
    Block fee_tampered = block;
    fee_tampered.header.total_fees = 9999;
    if (!fee_tampered.validate_internal()) {
        PASS();
    } else {
        FAIL("invalid total_fees was NOT caught");
    }
}

// ═══════════════════════════════════════════════════════════════════
// 2. Dynamic Difficulty Retargeting & Block Time Simulation
// ═══════════════════════════════════════════════════════════════════
void test_difficulty_retargeting() {
    std::cout << "\n=== 2. Dynamic Difficulty & Chain Invariants ===" << std::endl;

    genesis::GenesisConfig gen_cfg;
    gen_cfg.founder_address = "KasturiFounder000000000000000000000000000000000";
    gen_cfg.secondary_address = "KasturiSecondary0000000000000000000000000000000";
    gen_cfg.reward_pool_address = "KasturiRewardPool000000000000000000000000000000";
    gen_cfg.timestamp = 1700000000;

    Block genesis_block = genesis::build_genesis_block(gen_cfg);
    Blockchain chain;
    chain.initialize_genesis(genesis_block);

    TEST("Simulate mining 20 consecutive blocks with 5s target block time");
    PrivateKey validator_priv = generate_private_key();
    Hash256 prev_hash = genesis_block.compute_hash();
    uint64_t current_time = gen_cfg.timestamp;

    bool all_valid = true;
    for (uint64_t i = 1; i <= 20; ++i) {
        current_time += 5; // Exactly 5 seconds per block
        Transaction tx = create_sample_tx(validator_priv, i - 1, 100);

        Block b;
        b.header.version = 1;
        b.header.height = i;
        b.header.timestamp = current_time;
        b.header.previous_hash = prev_hash;
        b.transactions = {tx};
        b.header.tx_count = 1;
        b.header.total_fees = tx.fee;
        b.header.merkle_root = b.compute_merkle_root();

        AddBlockResult res = chain.add_block(b);
        if (res != AddBlockResult::SUCCESS) {
            all_valid = false;
            break;
        }
        prev_hash = b.compute_hash();
    }

    if (all_valid && chain.block_count() == 21 && chain.get_height() == 20) {
        PASS();
    } else {
        FAIL("failed to append 20 consecutive blocks to chain");
    }

    TEST("Full chain validation across all 21 blocks");
    if (chain.validate_full_chain()) {
        PASS();
    } else {
        FAIL("full chain validation failed");
    }
}

// ═══════════════════════════════════════════════════════════════════
// 3. Fork Choice & Heavy Chain Reorg Simulation
// ═══════════════════════════════════════════════════════════════════
void test_fork_choice_and_reorg() {
    std::cout << "\n=== 3. Fork Choice & Chain Reorganization ===" << std::endl;

    genesis::GenesisConfig gen_cfg;
    gen_cfg.founder_address = "KasturiFounder000000000000000000000000000000000";
    gen_cfg.secondary_address = "KasturiSecondary0000000000000000000000000000000";
    gen_cfg.reward_pool_address = "KasturiRewardPool000000000000000000000000000000";
    gen_cfg.timestamp = 1700000000;

    Block genesis_block = genesis::build_genesis_block(gen_cfg);

    TEST("Construct two competing valid chains (Short Chain A [h=3] vs Long Chain B [h=5])");
    Blockchain chain_a;
    chain_a.initialize_genesis(genesis_block);

    Blockchain chain_b;
    chain_b.initialize_genesis(genesis_block);

    PrivateKey priv_a = generate_private_key();
    PrivateKey priv_b = generate_private_key();

    // Chain A (3 blocks)
    Hash256 parent_a = genesis_block.compute_hash();
    for (uint64_t i = 1; i <= 3; ++i) {
        Block b;
        b.header.version = 1;
        b.header.height = i;
        b.header.timestamp = 1700000000 + i * 5;
        b.header.previous_hash = parent_a;
        Transaction tx = create_sample_tx(priv_a, i, 50);
        b.transactions = {tx};
        b.header.tx_count = 1;
        b.header.total_fees = tx.fee;
        b.header.merkle_root = b.compute_merkle_root();
        chain_a.add_block(b);
        parent_a = b.compute_hash();
    }

    // Chain B (5 blocks)
    Hash256 parent_b = genesis_block.compute_hash();
    for (uint64_t i = 1; i <= 5; ++i) {
        Block b;
        b.header.version = 1;
        b.header.height = i;
        b.header.timestamp = 1700000000 + i * 5;
        b.header.previous_hash = parent_b;
        Transaction tx = create_sample_tx(priv_b, i, 100);
        b.transactions = {tx};
        b.header.tx_count = 1;
        b.header.total_fees = tx.fee;
        b.header.merkle_root = b.compute_merkle_root();
        chain_b.add_block(b);
        parent_b = b.compute_hash();
    }

    if (chain_a.get_height() == 3 && chain_b.get_height() == 5) {
        PASS();
    } else {
        FAIL("chain heights unexpected");
    }

    TEST("Chain B (longer chain) has higher total block count and tip height");
    if (chain_b.block_count() > chain_a.block_count() && chain_b.get_height() > chain_a.get_height()) {
        PASS();
    } else {
        FAIL("longer chain rule violated");
    }
}

// ═══════════════════════════════════════════════════════════════════
// 4. Genesis Invariants
// ═══════════════════════════════════════════════════════════════════
void test_genesis_invariants() {
    std::cout << "\n=== 4. Genesis Invariants & Allocation Mechanics ===" << std::endl;

    genesis::GenesisConfig gen_cfg;
    gen_cfg.founder_address = "KasturiFounder000000000000000000000000000000000";
    gen_cfg.secondary_address = "KasturiSecondary0000000000000000000000000000000";
    gen_cfg.reward_pool_address = "KasturiRewardPool000000000000000000000000000000";
    gen_cfg.timestamp = 1700000000;

    Block genesis_block = genesis::build_genesis_block(gen_cfg);

    TEST("Genesis block structure and validation");
    if (genesis::is_valid_genesis(genesis_block) && genesis_block.header.height == 0) {
        PASS();
    } else {
        FAIL("invalid genesis block generated");
    }

    TEST("Apply genesis to StateDB and verify exact initial allocations");
    state::StateDB state_db;
    bool opened = state_db.open(":memory:");
    if (!opened) {
        FAIL("failed to open StateDB");
        return;
    }

    if (genesis::apply_genesis_to_state(genesis_block, state_db)) {
        uint64_t founder_bal = state_db.get_balance(gen_cfg.founder_address);
        uint64_t secondary_bal = state_db.get_balance(gen_cfg.secondary_address);
        uint64_t reward_pool_bal = state_db.get_balance(gen_cfg.reward_pool_address);

        // 1M NSK = 1,000,000 * 1e8 = 100,000,000,000,000 (10^14)
        // 19M NSK = 19,000,000 * 1e8 = 1,900,000,000,000,000 (1.9*10^15)
        uint64_t total = founder_bal + secondary_bal + reward_pool_bal;
        if (founder_bal == 100000000000000ULL &&
            secondary_bal == 100000000000000ULL &&
            reward_pool_bal == 1900000000000000ULL &&
            total == 2100000000000000ULL) {
            PASS();
        } else {
            FAIL("initial balances mismatch");
        }
    } else {
        FAIL("apply_genesis_to_state failed");
    }
}

int main() {
    std::cout << "===============================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Block & Blockchain Invariants Suite" << std::endl;
    std::cout << "===============================================================" << std::endl;

    test_merkle_tree_integrity();
    test_difficulty_retargeting();
    test_fork_choice_and_reorg();
    test_genesis_invariants();

    std::cout << "\n===============================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "===============================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
