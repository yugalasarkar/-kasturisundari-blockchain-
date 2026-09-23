// Kasturisundari Chain — Memory Pool (Mempool)
// Holds unconfirmed transactions before they are mined into a block.
// Handles validation: signature checking, nonce checking, and double-spending prevention.

#pragma once

#include "kasturisundari/core/transaction.h"
#include "kasturisundari/state/state_db.h"

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace kasturisundari {
namespace mempool {

/// Reasons why a transaction might be rejected by the mempool.
enum class TxRejectReason {
    NONE,
    INVALID_SIGNATURE,
    INVALID_NONCE,
    INSUFFICIENT_FUNDS,
    DUPLICATE_TRANSACTION,
    FEE_TOO_LOW,
    INVALID_FORMAT
};

/// Convert TxRejectReason to a human-readable string.
const char* reject_reason_to_string(TxRejectReason reason);

/// The Memory Pool (Mempool) class.
class Mempool {
public:
    Mempool() = default;

    /// Attempt to add a transaction to the mempool.
    /// Validates the transaction against the current state DB and other pending txs.
    /// Returns NONE if accepted, otherwise the rejection reason.
    TxRejectReason add_transaction(const core::Transaction& tx, const state::StateDB& state);

    /// Get all valid pending transactions, sorted by fee (highest first).
    /// Limits the result to `max_count` (useful for building a block).
    std::vector<core::Transaction> get_pending_transactions(size_t max_count = 5000) const;

    /// Remove transactions that were included in a newly mined block.
    void remove_transactions(const std::vector<core::Transaction>& txs);

    /// Check if a transaction is currently in the mempool.
    bool has_transaction(const std::string& txid) const;

    /// Get the total number of transactions in the mempool.
    size_t size() const;

    /// Clear the entire mempool.
    void clear();

    /// Get the expected nonce for an address (taking pending txs into account).
    uint64_t get_expected_nonce(const std::string& address, const state::StateDB& state) const;

private:
    mutable std::mutex mtx_;

    // Map: txid -> Transaction
    std::unordered_map<std::string, core::Transaction> pending_txs_;

    // To prevent double spending within the mempool, we track pending balances and nonces.
    // Map: address -> {pending_deduction, highest_nonce}
    struct PendingState {
        uint64_t pending_deduction = 0;
        uint64_t highest_nonce = 0;
    };
    std::unordered_map<std::string, PendingState> address_states_;
};

} // namespace mempool
} // namespace kasturisundari
