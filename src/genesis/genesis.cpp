// Kasturisundari Chain — Genesis Block Builder Implementation

#include "kasturisundari/genesis/genesis.h"
#include "kasturisundari/economics/tokenomics.h"

namespace kasturisundari {
namespace genesis {

core::Block build_genesis_block(const GenesisConfig& config) {
    using namespace economics;

    // Create the three genesis transactions:
    //   1. Allocate 1,000,000 NSK to the founder
    //   2. Allocate 1,000,000 NSK to the secondary wallet
    //   3. Lock 19,000,000 NSK in the reward pool

    core::Transaction tx_founder;
    tx_founder.version   = 1;
    tx_founder.type      = core::TxType::TRANSFER;
    tx_founder.timestamp = config.timestamp;
    tx_founder.nonce     = 0;
    tx_founder.sender    = "GENESIS";
    tx_founder.recipient = config.founder_address;
    tx_founder.amount    = FOUNDER_ALLOCATION;
    tx_founder.fee       = 0;

    core::Transaction tx_secondary;
    tx_secondary.version   = 1;
    tx_secondary.type      = core::TxType::TRANSFER;
    tx_secondary.timestamp = config.timestamp;
    tx_secondary.nonce     = 1;
    tx_secondary.sender    = "GENESIS";
    tx_secondary.recipient = config.secondary_address;
    tx_secondary.amount    = SECONDARY_ALLOCATION;
    tx_secondary.fee       = 0;

    core::Transaction tx_pool;
    tx_pool.version   = 1;
    tx_pool.type      = core::TxType::TRANSFER;
    tx_pool.timestamp = config.timestamp;
    tx_pool.nonce     = 2;
    tx_pool.sender    = "GENESIS";
    tx_pool.recipient = config.reward_pool_address;
    tx_pool.amount    = INITIAL_REWARD_POOL;
    tx_pool.fee       = 0;

    std::vector<core::Transaction> txs = {tx_founder, tx_secondary, tx_pool};

    // Build the block.
    core::Block genesis;
    genesis.transactions = txs;

    genesis.header.version          = 1;
    genesis.header.height           = 0;
    genesis.header.previous_hash    = crypto::Hash256{};  // All zeros
    genesis.header.merkle_root      = genesis.compute_merkle_root();
    genesis.header.attestation_root = crypto::Hash256{};
    genesis.header.timestamp        = config.timestamp;
    genesis.header.tx_count         = static_cast<uint32_t>(txs.size());
    genesis.header.total_fees       = 0;  // No fees in genesis

    return genesis;
}

bool apply_genesis_to_state(const core::Block& genesis, state::StateDB& db) {
    using namespace economics;

    if (!is_valid_genesis(genesis)) return false;

    // Apply each genesis transaction.
    for (const auto& tx : genesis.transactions) {
        state::Account acc = db.get_account(tx.recipient);
        acc.balance += tx.amount;
        if (!db.put_account(tx.recipient, acc)) return false;
    }

    // Store chain metadata.
    db.put_chain_height(0);
    db.put_latest_block_hash(genesis.compute_hash());

    return true;
}

bool is_valid_genesis(const core::Block& block) {
    // Must be at height 0.
    if (block.header.height != 0) return false;

    // Previous hash must be all zeros.
    crypto::Hash256 zeros{};
    if (block.header.previous_hash != zeros) return false;

    // Must have exactly 3 transactions.
    if (block.transactions.size() != 3) return false;

    // All senders must be "GENESIS".
    for (const auto& tx : block.transactions) {
        if (tx.sender != "GENESIS") return false;
    }

    // Total amount must equal MAX_SUPPLY.
    uint64_t total = 0;
    for (const auto& tx : block.transactions) {
        total += tx.amount;
    }
    if (total != economics::MAX_SUPPLY) return false;

    // No fees in genesis.
    if (block.header.total_fees != 0) return false;

    // Internal consistency.
    if (!block.validate_internal()) return false;

    return true;
}

} // namespace genesis
} // namespace kasturisundari
