// Kasturisundari Chain — Phase 1 Core Tests
// Tests Transactions, Blocks, Merkle Trees, and Blockchain validation.

#include "kasturisundari/core/blockchain.h"
#include "kasturisundari/core/block.h"
#include "kasturisundari/core/transaction.h"
#include "kasturisundari/crypto/address.h"
#include "kasturisundari/crypto/sha256.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>

using namespace kasturisundari::core;
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

// ─── Helpers ────────────────────────────────────────────────────────

uint64_t now_timestamp() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count());
}

Transaction make_signed_tx(const KeyPair& sender, const std::string& recipient,
                           uint64_t amount, uint64_t fee, uint64_t nonce) {
    Transaction tx;
    tx.version   = 1;
    tx.type      = TxType::TRANSFER;
    tx.timestamp = now_timestamp();
    tx.nonce     = nonce;
    tx.sender    = sender.address;
    tx.recipient = recipient;
    tx.amount    = amount;
    tx.fee       = fee;
    tx.sign_transaction(sender.private_key);
    return tx;
}

Block make_block(uint64_t height, const Hash256& prev_hash,
                 const std::vector<Transaction>& txs) {
    Block blk;
    blk.transactions = txs;

    // Compute fees.
    uint64_t total_fees = 0;
    for (const auto& tx : txs) total_fees += tx.fee;

    // Build header.
    blk.header.version          = 1;
    blk.header.height           = height;
    blk.header.previous_hash    = prev_hash;
    blk.header.merkle_root      = blk.compute_merkle_root();
    blk.header.attestation_root = Hash256{};  // No attestations in these test blocks.
    blk.header.timestamp        = now_timestamp();
    blk.header.tx_count         = static_cast<uint32_t>(txs.size());
    blk.header.total_fees       = total_fees;

    return blk;
}

// ═══════════════════════════════════════════════════════════════════
// Transaction Tests
// ═══════════════════════════════════════════════════════════════════

void test_transaction() {
    std::cout << "\n=== Transaction Tests ===" << std::endl;

    KeyPair alice = generate_keypair();
    KeyPair bob   = generate_keypair();

    TEST("Create and sign a transaction");
    Transaction tx = make_signed_tx(alice, bob.address, 100000000, 1000, 0);
    if (tx.sender == alice.address && tx.recipient == bob.address) {
        PASS();
    } else {
        FAIL("addresses mismatch");
    }

    TEST("Verify transaction signature");
    if (tx.verify_signature()) {
        PASS();
    } else {
        FAIL("valid signature was rejected");
    }

    TEST("TXID is a 64-char hex string");
    std::string txid = tx.txid();
    if (txid.size() == 64) {
        PASS();
    } else {
        FAIL("txid length is " + std::to_string(txid.size()));
    }

    TEST("Same transaction always produces the same TXID");
    if (tx.txid() == txid) {
        PASS();
    } else {
        FAIL("txid changed between calls");
    }

    TEST("Tampered amount invalidates signature");
    Transaction tampered = tx;
    tampered.amount = 999999999;
    if (!tampered.verify_signature()) {
        PASS();
    } else {
        FAIL("tampered tx was accepted");
    }

    TEST("Transaction serialize/deserialize round-trip");
    auto bytes = tx.serialize();
    Transaction recovered = Transaction::deserialize(bytes);
    if (recovered.txid() == tx.txid()
        && recovered.sender == tx.sender
        && recovered.recipient == tx.recipient
        && recovered.amount == tx.amount
        && recovered.verify_signature()) {
        PASS();
    } else {
        FAIL("round-trip data mismatch");
    }

    TEST("TxType string conversion");
    if (std::string(tx_type_to_string(TxType::TRANSFER)) == "Transfer"
        && std::string(tx_type_to_string(TxType::ATTESTATION)) == "Attestation"
        && std::string(tx_type_to_string(TxType::CONTRIBUTION_PROPOSAL)) == "ContributionProposal") {
        PASS();
    } else {
        FAIL("type names mismatch");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Block Tests
// ═══════════════════════════════════════════════════════════════════

void test_block() {
    std::cout << "\n=== Block Tests ===" << std::endl;

    KeyPair alice = generate_keypair();
    KeyPair bob   = generate_keypair();

    Transaction tx1 = make_signed_tx(alice, bob.address, 50000000, 1000, 0);
    Transaction tx2 = make_signed_tx(alice, bob.address, 30000000, 2000, 1);

    Hash256 prev_hash{};  // Genesis previous = zeros.
    Block blk = make_block(0, prev_hash, {tx1, tx2});

    TEST("Block has correct tx count");
    if (blk.header.tx_count == 2) {
        PASS();
    } else {
        FAIL("tx_count = " + std::to_string(blk.header.tx_count));
    }

    TEST("Block total_fees is sum of tx fees");
    if (blk.header.total_fees == 3000) {
        PASS();
    } else {
        FAIL("total_fees = " + std::to_string(blk.header.total_fees));
    }

    TEST("Block validates internally");
    if (blk.validate_internal()) {
        PASS();
    } else {
        FAIL("internal validation failed");
    }

    TEST("Block ID is a 64-char hex string");
    if (blk.block_id().size() == 64) {
        PASS();
    } else {
        FAIL("block_id length wrong");
    }

    TEST("Merkle root changes when a transaction is modified");
    Block tampered = blk;
    tampered.transactions[0].amount = 99999;
    Hash256 tampered_root = tampered.compute_merkle_root();
    if (tampered_root != blk.header.merkle_root) {
        PASS();
    } else {
        FAIL("merkle root should have changed");
    }

    TEST("Tampered block fails internal validation");
    if (!tampered.validate_internal()) {
        PASS();
    } else {
        FAIL("tampered block passed validation");
    }

    TEST("Block serialize/deserialize round-trip");
    auto bytes = blk.serialize();
    Block recovered = Block::deserialize(bytes);
    if (recovered.block_id() == blk.block_id()
        && recovered.header.tx_count == blk.header.tx_count
        && recovered.validate_internal()) {
        PASS();
    } else {
        FAIL("round-trip data mismatch");
    }
}

// ═══════════════════════════════════════════════════════════════════
// Blockchain Tests
// ═══════════════════════════════════════════════════════════════════

void test_blockchain() {
    std::cout << "\n=== Blockchain Tests ===" << std::endl;

    KeyPair alice   = generate_keypair();
    KeyPair bob     = generate_keypair();
    KeyPair charlie = generate_keypair();

    // ─── Genesis Block ──────────────────────────────────────────
    Transaction genesis_tx = make_signed_tx(alice, bob.address, 1000000, 0, 0);
    Hash256 zeros{};
    Block genesis = make_block(0, zeros, {genesis_tx});

    Blockchain chain;
    chain.initialize_genesis(genesis);

    TEST("Chain initialized with genesis");
    if (chain.block_count() == 1 && chain.get_height() == 0) {
        PASS();
    } else {
        FAIL("block count or height wrong");
    }

    TEST("Genesis block is retrievable by height");
    auto g = chain.get_block_at(0);
    if (g.has_value() && g->get().block_id() == genesis.block_id()) {
        PASS();
    } else {
        FAIL("genesis not found at height 0");
    }

    TEST("Genesis block is retrievable by hash");
    auto gh = chain.get_block_by_hash(genesis.compute_hash());
    if (gh.has_value()) {
        PASS();
    } else {
        FAIL("genesis not found by hash");
    }

    // ─── Block 1 ────────────────────────────────────────────────
    Transaction tx1 = make_signed_tx(bob, charlie.address, 500000, 100, 0);
    Block block1 = make_block(1, genesis.compute_hash(), {tx1});

    TEST("Add valid block 1");
    auto result = chain.add_block(block1);
    if (result == AddBlockResult::SUCCESS) {
        PASS();
    } else {
        FAIL(add_block_result_to_string(result));
    }

    // ─── Block 2 ────────────────────────────────────────────────
    Transaction tx2a = make_signed_tx(alice, charlie.address, 200000, 50, 1);
    Transaction tx2b = make_signed_tx(charlie, bob.address, 100000, 75, 0);
    Block block2 = make_block(2, block1.compute_hash(), {tx2a, tx2b});

    TEST("Add valid block 2 with multiple transactions");
    result = chain.add_block(block2);
    if (result == AddBlockResult::SUCCESS) {
        PASS();
    } else {
        FAIL(add_block_result_to_string(result));
    }

    TEST("Chain height is 2");
    if (chain.get_height() == 2) {
        PASS();
    } else {
        FAIL("height = " + std::to_string(chain.get_height()));
    }

    TEST("Block count is 3");
    if (chain.block_count() == 3) {
        PASS();
    } else {
        FAIL("count = " + std::to_string(chain.block_count()));
    }

    // ─── Validation Tests ───────────────────────────────────────

    TEST("Full chain validation passes");
    if (chain.validate_full_chain()) {
        PASS();
    } else {
        FAIL("valid chain failed validation");
    }

    TEST("Total recycled fees are correct");
    // genesis: 0, block1: 100, block2: 50+75=125
    if (chain.get_total_recycled_fees() == 225) {
        PASS();
    } else {
        FAIL("fees = " + std::to_string(chain.get_total_recycled_fees()));
    }

    TEST("Contains transaction works");
    if (chain.contains_transaction(tx1.compute_hash())) {
        PASS();
    } else {
        FAIL("existing tx not found");
    }

    TEST("Contains transaction rejects unknown hash");
    Hash256 fake_hash = sha256(std::string("nonexistent"));
    if (!chain.contains_transaction(fake_hash)) {
        PASS();
    } else {
        FAIL("fake tx was found");
    }

    // ─── Rejection Tests ────────────────────────────────────────

    TEST("Reject block with wrong previous hash");
    Block bad_prev = make_block(3, zeros, {});
    result = chain.add_block(bad_prev);
    if (result == AddBlockResult::INVALID_PREVIOUS_HASH) {
        PASS();
    } else {
        FAIL(add_block_result_to_string(result));
    }

    TEST("Reject block with wrong height");
    Block bad_height = make_block(99, block2.compute_hash(), {});
    result = chain.add_block(bad_height);
    if (result == AddBlockResult::INVALID_HEIGHT) {
        PASS();
    } else {
        FAIL(add_block_result_to_string(result));
    }

    TEST("Reject duplicate block");
    result = chain.add_block(block2);
    if (result == AddBlockResult::DUPLICATE_BLOCK) {
        PASS();
    } else {
        FAIL(add_block_result_to_string(result));
    }

    // ─── Display ────────────────────────────────────────────────
    std::cout << "\n--- Sample Kasturisundari Chain ---" << std::endl;
    for (size_t i = 0; i < chain.block_count(); ++i) {
        auto b = chain.get_block_at(i);
        if (b) {
            const auto& blk = b->get();
            std::cout << "  Block #" << blk.header.height
                      << " | Hash: " << blk.block_id().substr(0, 16) << "..."
                      << " | Txs: " << blk.header.tx_count
                      << " | Fees: " << blk.header.total_fees
                      << std::endl;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════
int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 1 Core Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_transaction();
    test_block();
    test_blockchain();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
