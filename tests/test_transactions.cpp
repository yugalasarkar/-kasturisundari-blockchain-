// Kasturisundari Chain — Transaction Validation Test Suite
// Rigorous verification for Phase 2: Binary serialization roundtrips across all TxTypes,
// Nonce & double-spend prevention, gas/fee mechanics, and Mempool invariants.

#include "kasturisundari/core/transaction.h"
#include "kasturisundari/crypto/address.h"
#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include "kasturisundari/crypto/sha256.h"
#include "kasturisundari/mempool/mempool.h"
#include "kasturisundari/state/state_db.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace kasturisundari;
using namespace kasturisundari::core;
using namespace kasturisundari::crypto;
using namespace kasturisundari::mempool;

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

// ═══════════════════════════════════════════════════════════════════
// 1. Binary Serialization & Deserialization Roundtrips
// ═══════════════════════════════════════════════════════════════════
void test_transaction_serialization_roundtrips() {
    std::cout << "\n=== 1. Transaction Serialization Roundtrips ===" << std::endl;

    PrivateKey priv = generate_private_key();
    PublicKey pub = *derive_public_key(priv);
    std::string sender_addr = public_key_to_address(pub);

    std::vector<TxType> types_to_test = {
        TxType::TRANSFER,
        TxType::ATTESTATION,
        TxType::CONTRIBUTION_PROPOSAL,
        TxType::CONTRIBUTION_APPROVAL,
        TxType::FEE_RECYCLE,
        TxType::CONTRACT_DEPLOY,
        TxType::CONTRACT_CALL,
        TxType::GOVERNANCE_VOTE
    };

    TEST("Verify binary serialization roundtrip across all 8 TxTypes");
    bool all_passed = true;

    for (size_t i = 0; i < types_to_test.size(); ++i) {
        Transaction tx;
        tx.version = 1;
        tx.type = types_to_test[i];
        tx.timestamp = 1700000000 + i;
        tx.nonce = i + 1;
        tx.sender = sender_addr;
        tx.recipient = "Kasturi2222222222222222222222222222222222222222";
        tx.amount = 1000 * (i + 1);
        tx.fee = 50;
        tx.data = {static_cast<uint8_t>(i), 0xAA, 0xBB, 0xCC};

        bool signed_ok = tx.sign_transaction(priv);
        bool verify_ok = tx.verify_signature();
        if (!signed_ok || !verify_ok) {
            std::cout << "[DEBUG tx_ser] Signing/verification failed before serialization" << std::endl;
            all_passed = false;
            break;
        }

        std::vector<uint8_t> serialized = tx.serialize();
        Transaction deserialized = Transaction::deserialize(serialized);

        if (deserialized.version != tx.version ||
            deserialized.type != tx.type ||
            deserialized.nonce != tx.nonce ||
            deserialized.sender != tx.sender ||
            deserialized.recipient != tx.recipient ||
            deserialized.amount != tx.amount ||
            deserialized.fee != tx.fee ||
            deserialized.data != tx.data ||
            deserialized.compute_hash() != tx.compute_hash() ||
            !deserialized.verify_signature()) {
            all_passed = false;
            break;
        }
    }

    if (all_passed) {
        PASS();
    } else {
        FAIL("serialization roundtrip failed for one or more TxTypes");
    }
}

// ═══════════════════════════════════════════════════════════════════
// 2. Nonce & Double-Spend Protection
// ═══════════════════════════════════════════════════════════════════
void test_nonce_and_double_spend_protection() {
    std::cout << "\n=== 2. Nonce Tracking & Double-Spend Protection ===" << std::endl;

    state::StateDB db;
    if (!db.open(":memory:")) {
        FAIL("failed to open StateDB");
        return;
    }

    PrivateKey priv = generate_private_key();
    PublicKey pub = *derive_public_key(priv);
    std::string sender_addr = public_key_to_address(pub);

    // Fund the sender account
    state::Account acc;
    acc.balance = 50000;
    acc.nonce = 0;
    db.put_account(sender_addr, acc);

    Mempool mempool;

    TEST("Accept valid sequential transaction (nonce 0)");
    Transaction tx0;
    tx0.version = 1;
    tx0.type = TxType::TRANSFER;
    tx0.timestamp = 1700000000;
    tx0.nonce = 0;
    tx0.sender = sender_addr;
    tx0.recipient = "KasturiRecipient000000000000000000000000000";
    tx0.amount = 1000;
    tx0.fee = 10;
    tx0.sign_transaction(priv);

    TxRejectReason r0 = mempool.add_transaction(tx0, db);
    if (r0 == TxRejectReason::NONE && mempool.size() == 1) {
        PASS();
    } else {
        FAIL(reject_reason_to_string(r0));
    }

    TEST("Reject duplicate transaction submission");
    TxRejectReason r_dup = mempool.add_transaction(tx0, db);
    if (r_dup == TxRejectReason::DUPLICATE_TRANSACTION) {
        PASS();
    } else {
        FAIL("duplicate transaction was NOT rejected");
    }

    TEST("Reject out-of-order nonce (nonce 5 when expecting 1)");
    Transaction tx5 = tx0;
    tx5.nonce = 5;
    tx5.sign_transaction(priv);
    TxRejectReason r_nonce = mempool.add_transaction(tx5, db);
    if (r_nonce == TxRejectReason::INVALID_NONCE) {
        PASS();
    } else {
        FAIL("out-of-order nonce was NOT rejected");
    }
}

// ═══════════════════════════════════════════════════════════════════
// 3. Gas Mechanics & Insufficient Balance Protection
// ═══════════════════════════════════════════════════════════════════
void test_gas_and_balance_mechanics() {
    std::cout << "\n=== 3. Gas Mechanics & Balance Deduction Checks ===" << std::endl;

    state::StateDB db;
    if (!db.open(":memory:")) {
        FAIL("failed to open StateDB");
        return;
    }

    PrivateKey priv = generate_private_key();
    PublicKey pub = *derive_public_key(priv);
    std::string sender_addr = public_key_to_address(pub);

    // Fund account with only 100 units
    state::Account acc;
    acc.balance = 100;
    acc.nonce = 0;
    db.put_account(sender_addr, acc);

    Mempool mempool;

    TEST("Reject transaction exceeding account balance (amount + fee > balance)");
    Transaction tx_poor;
    tx_poor.version = 1;
    tx_poor.type = TxType::TRANSFER;
    tx_poor.timestamp = 1700000000;
    tx_poor.nonce = 0;
    tx_poor.sender = sender_addr;
    tx_poor.recipient = "KasturiRecipient000000000000000000000000000";
    tx_poor.amount = 200; // Exceeds balance of 100
    tx_poor.fee = 10;
    tx_poor.sign_transaction(priv);

    TxRejectReason r_funds = mempool.add_transaction(tx_poor, db);
    if (r_funds == TxRejectReason::INSUFFICIENT_FUNDS) {
        PASS();
    } else {
        FAIL("insufficient funds transaction was NOT rejected");
    }

    TEST("Reject transaction with invalid signature");
    Transaction tx_invalid_sig = tx_poor;
    tx_invalid_sig.amount = 50; // Within balance
    tx_invalid_sig.sign_transaction(priv);
    tx_invalid_sig.signature[0] ^= 0xFF; // Corrupt signature

    TxRejectReason r_sig = mempool.add_transaction(tx_invalid_sig, db);
    if (r_sig == TxRejectReason::INVALID_SIGNATURE) {
        PASS();
    } else {
        FAIL("invalid signature was NOT rejected");
    }
}

int main() {
    std::cout << "===============================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Transaction Validation Test Suite" << std::endl;
    std::cout << "===============================================================" << std::endl;

    test_transaction_serialization_roundtrips();
    test_nonce_and_double_spend_protection();
    test_gas_and_balance_mechanics();

    std::cout << "\n===============================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "===============================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
