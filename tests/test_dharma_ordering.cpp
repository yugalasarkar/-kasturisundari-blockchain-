// Kasturisundari Chain — Phase 2: Anti-MEV Dharma Fair Ordering Test
#include "kasturisundari/mempool/mempool.h"
#include "kasturisundari/state/state_db.h"
#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace kasturisundari;

void test_dharma_fcfs_ordering() {
    std::cout << "[Test Dharma Ordering] Running Anti-MEV FCFS ordering test..." << std::endl;
    
    state::StateDB state;
    state.open(":memory:");

    crypto::PrivateKey privKeyA = crypto::generate_private_key();
    crypto::PrivateKey privKeyB = crypto::generate_private_key();

    std::string addrA = "K" + crypto::hash_to_hex(crypto::sha256((const uint8_t*)"senderA", 7)).substr(0, 40);
    std::string addrB = "K" + crypto::hash_to_hex(crypto::sha256((const uint8_t*)"senderB", 7)).substr(0, 40);
    std::string poolAddr = "K0000000000000000000000000000000000000000";

    // Fund both accounts in state
    state::Account accA{ 100000000, 0 };
    state::Account accB{ 100000000, 0 };
    state.put_account(addrA, accA);
    state.put_account(addrB, accB);

    mempool::Mempool mp;

    // Tx B: Low fee (100 NSK), submitted EARLIER (T0 = 1000ns)
    core::Transaction txB;
    txB.version = 1;
    txB.type = core::TxType::TRANSFER;
    txB.timestamp = 1000;
    txB.nonce = 0;
    txB.sender = addrB;
    txB.recipient = poolAddr;
    txB.amount = 1000;
    txB.fee = 100; // Low fee
    txB.arrival_timestamp_ns = 1000;
    txB.sign_transaction(privKeyB);

    // Tx A: High fee (100000 NSK - Front-running attempt!), submitted LATER (T1 = 2000ns)
    core::Transaction txA;
    txA.version = 1;
    txA.type = core::TxType::TRANSFER;
    txA.timestamp = 2000;
    txA.nonce = 0;
    txA.sender = addrA;
    txA.recipient = poolAddr;
    txA.amount = 5000;
    txA.fee = 100000; // Extremely high PGA fee tip!
    txA.arrival_timestamp_ns = 2000; // Arrived later!
    txA.sign_transaction(privKeyA);

    // Add both to mempool
    auto resB = mp.add_transaction(txB, state);
    assert(resB == mempool::TxRejectReason::NONE);
    
    auto resA = mp.add_transaction(txA, state);
    assert(resA == mempool::TxRejectReason::NONE);

    // Retrieve pending transactions for block forging
    auto pending = mp.get_pending_transactions(10);
    assert(pending.size() == 2);

    // VERIFICATION: First transaction in pending MUST be TxB (earlier arrival T0), NOT TxA!
    assert(pending[0].sender == addrB);
    assert(pending[1].sender == addrA);

    std::cout << "  ✓ Dharma Ordering successfully prioritized TxB (arrival: 1000ns, fee: 100) over TxA (arrival: 2000ns, fee: 100000)." << std::endl;
    std::cout << "  ✓ Priority Gas Auction (PGA) front-running attempt ELIMINATED." << std::endl;
}

int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "  KasturiChain Phase 2: Dharma Anti-MEV Test    " << std::endl;
    std::cout << "=================================================" << std::endl;

    test_dharma_fcfs_ordering();

    std::cout << "\n✅ PHASE 2 DHARMA ANTI-MEV ORDERING PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
