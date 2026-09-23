// Kasturisundari Chain — Phase 3 Mempool Tests
// Tests transaction validation, double spending prevention, and nonce checking.

#include "kasturisundari/mempool/mempool.h"
#include "kasturisundari/state/state_db.h"
#include "kasturisundari/crypto/address.h"
#include "kasturisundari/economics/tokenomics.h"

#include <chrono>
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

// Helper to create a signed transaction
core::Transaction make_signed_tx(const crypto::KeyPair& sender, const std::string& recipient,
                                 uint64_t amount, uint64_t fee, uint64_t nonce) {
    core::Transaction tx;
    tx.version   = 1;
    tx.type      = core::TxType::TRANSFER;
    tx.timestamp = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
    tx.nonce     = nonce;
    tx.sender    = sender.address;
    tx.recipient = recipient;
    tx.amount    = amount;
    tx.fee       = fee;
    tx.sign_transaction(sender.private_key);
    return tx;
}

void test_mempool() {
    std::cout << "\n=== Mempool & Validation Tests ===" << std::endl;

    // 1. Setup a test state DB.
    std::string db_path = "./test_mempool_db";
    std::filesystem::remove_all(db_path);
    state::StateDB state;
    state.open(db_path);

    // Create users.
    crypto::KeyPair alice = crypto::generate_keypair();
    crypto::KeyPair bob   = crypto::generate_keypair();

    // Give Alice 1,000,000 NSK, nonce starts at 0.
    state::Account acc_alice{1'000'000ULL * economics::UNITS_PER_NILA, 0};
    state.put_account(alice.address, acc_alice);
    
    // Give Bob some NSK too
    state::Account acc_bob{1000ULL * economics::UNITS_PER_NILA, 0};
    state.put_account(bob.address, acc_bob);

    mempool::Mempool pool;

    TEST("Accept valid transaction");
    core::Transaction tx1 = make_signed_tx(alice, bob.address, 50'000ULL * economics::UNITS_PER_NILA, economics::MIN_TRANSACTION_FEE, 0);
    auto res = pool.add_transaction(tx1, state);
    if (res == mempool::TxRejectReason::NONE && pool.size() == 1) {
        PASS();
    } else {
        FAIL("rejected valid tx: " + std::string(mempool::reject_reason_to_string(res)));
    }

    TEST("Reject duplicate transaction");
    res = pool.add_transaction(tx1, state);
    if (res == mempool::TxRejectReason::DUPLICATE_TRANSACTION) {
        PASS();
    } else {
        FAIL("accepted duplicate tx");
    }

    TEST("Reject out-of-order nonce");
    // Alice's nonce should now be 1 inside the mempool.
    core::Transaction tx_bad_nonce = make_signed_tx(alice, bob.address, 1000, economics::MIN_TRANSACTION_FEE, 5); // Nonce 5 (expected 1)
    res = pool.add_transaction(tx_bad_nonce, state);
    if (res == mempool::TxRejectReason::INVALID_NONCE) {
        PASS();
    } else {
        FAIL("accepted out-of-order nonce");
    }

    TEST("Accept chained sequential nonce");
    core::Transaction tx2 = make_signed_tx(alice, bob.address, 10'000ULL * economics::UNITS_PER_NILA, economics::MIN_TRANSACTION_FEE, 1);
    res = pool.add_transaction(tx2, state);
    if (res == mempool::TxRejectReason::NONE && pool.size() == 2) {
        PASS();
    } else {
        FAIL("rejected valid sequential tx: " + std::string(mempool::reject_reason_to_string(res)));
    }

    TEST("Reject insufficient funds (accounting for pending)");
    // Alice has 1,000,000. She spent 60,000 + fees in pending txs. Try to spend 999,999.
    core::Transaction tx_poor = make_signed_tx(alice, bob.address, 999'999ULL * economics::UNITS_PER_NILA, economics::MIN_TRANSACTION_FEE, 2);
    res = pool.add_transaction(tx_poor, state);
    if (res == mempool::TxRejectReason::INSUFFICIENT_FUNDS) {
        PASS();
    } else {
        FAIL("accepted double-spend/overdraft");
    }

    TEST("Reject tampered signature");
    core::Transaction tx_tampered = make_signed_tx(alice, bob.address, 1000, economics::MIN_TRANSACTION_FEE, 2);
    tx_tampered.amount = 9999; // Change amount without resigning
    res = pool.add_transaction(tx_tampered, state);
    if (res == mempool::TxRejectReason::INVALID_SIGNATURE) {
        PASS();
    } else {
        FAIL("accepted tampered tx");
    }

    TEST("Reject fee too low");
    if (economics::MIN_TRANSACTION_FEE > 0) {
        core::Transaction tx_low_fee = make_signed_tx(alice, bob.address, 1000, economics::MIN_TRANSACTION_FEE - 1, 2);
        res = pool.add_transaction(tx_low_fee, state);
        if (res == mempool::TxRejectReason::FEE_TOO_LOW) {
            PASS();
        } else {
            FAIL("accepted low fee tx");
        }
    } else {
        // MIN_TRANSACTION_FEE is 0 (gasless transactions allowed for NILA transfer)
        PASS();
    }

    TEST("Reject sending to self");
    core::Transaction tx_self = make_signed_tx(alice, alice.address, 1000, economics::MIN_TRANSACTION_FEE, 2);
    res = pool.add_transaction(tx_self, state);
    if (res == mempool::TxRejectReason::INVALID_FORMAT) {
        PASS();
    } else {
        FAIL("accepted send to self");
    }

    TEST("Retrieve pending transactions sorted by fee");
    // Add one more with a high fee.
    core::Transaction tx3 = make_signed_tx(bob, alice.address, 1000, 100, 0); // Bob's first tx (fee=100)
    pool.add_transaction(tx3, state);
    
    auto pending = pool.get_pending_transactions();
    if (pending.size() == 3 && pending[0].txid() == tx3.txid()) {
        PASS();
    } else {
        FAIL("sorting failed");
    }

    TEST("Remove mined transactions");
    pool.remove_transactions({tx1, tx3});
    if (pool.size() == 1 && pool.has_transaction(tx2.txid())) {
        PASS();
    } else {
        FAIL("removal failed");
    }

    TEST("Clear mempool");
    pool.clear();
    if (pool.size() == 0) {
        PASS();
    } else {
        FAIL("mempool not cleared");
    }

    // Clean up
    state.close();
    std::filesystem::remove_all(db_path);
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 3 Mempool Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_mempool();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
