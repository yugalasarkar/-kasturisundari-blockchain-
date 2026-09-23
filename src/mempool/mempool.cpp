// Kasturisundari Chain — Mempool Implementation

#include "kasturisundari/mempool/mempool.h"
#include "kasturisundari/economics/tokenomics.h"

#include <iostream>
#include <algorithm>

namespace kasturisundari {
namespace mempool {

const char* reject_reason_to_string(TxRejectReason reason) {
    switch (reason) {
        case TxRejectReason::NONE:                  return "None";
        case TxRejectReason::INVALID_SIGNATURE:     return "InvalidSignature";
        case TxRejectReason::INVALID_NONCE:         return "InvalidNonce";
        case TxRejectReason::INSUFFICIENT_FUNDS:    return "InsufficientFunds";
        case TxRejectReason::DUPLICATE_TRANSACTION: return "DuplicateTransaction";
        case TxRejectReason::FEE_TOO_LOW:           return "FeeTooLow";
        case TxRejectReason::INVALID_FORMAT:        return "InvalidFormat";
        default:                                    return "Unknown";
    }
}

TxRejectReason Mempool::add_transaction(const core::Transaction& tx, const state::StateDB& state) {
    std::lock_guard<std::mutex> lock(mtx_);

    std::string txid = tx.txid();

    // 1. Check for duplicates.
    if (pending_txs_.find(txid) != pending_txs_.end()) {
        return TxRejectReason::DUPLICATE_TRANSACTION;
    }

    // 2. Minimum fee check.
    if (tx.fee < economics::MIN_TRANSACTION_FEE) {
        return TxRejectReason::FEE_TOO_LOW;
    }

    // 3. Format validation.
    if (tx.sender == tx.recipient) {
        return TxRejectReason::INVALID_FORMAT; // Can't send to self
    }

    // 4. Verify cryptographic signature.
    if (!tx.verify_signature()) {
        return TxRejectReason::INVALID_SIGNATURE;
    }

    // 5. State validation (Nonce & Balance).
    state::Account acc = state.get_account(tx.sender);
    
    // Get pending state for this sender within the mempool.
    uint64_t current_pending_deduction = 0;
    uint64_t expected_nonce = acc.nonce;

    auto it = address_states_.find(tx.sender);
    if (it != address_states_.end()) {
        current_pending_deduction = it->second.pending_deduction;
        expected_nonce = it->second.highest_nonce + 1;
    }

    // Nonce must match exactly to prevent replay and ensure ordering.
    if (tx.nonce != expected_nonce) {
        std::cout << "[Mempool] INVALID_NONCE: tx.nonce=" << tx.nonce 
                  << ", expected_nonce=" << expected_nonce 
                  << ", sender=" << tx.sender << std::endl;
        return TxRejectReason::INVALID_NONCE;
    }

    // Balance check. (total needed = amount + fee)
    // Prevent overflow in calculation.
    uint64_t total_needed = tx.amount + tx.fee;
    if (total_needed < tx.amount || total_needed < tx.fee) {
        return TxRejectReason::INVALID_FORMAT; // Overflow detected
    }

    uint64_t total_required_with_pending = current_pending_deduction + total_needed;
    if (total_required_with_pending > acc.balance) {
        return TxRejectReason::INSUFFICIENT_FUNDS;
    }

    // 6. Assign nanosecond arrival timestamp if not set
    if (tx.arrival_timestamp_ns == 0) {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        const_cast<core::Transaction&>(tx).arrival_timestamp_ns = ns;
    }

    // Accept transaction.
    pending_txs_[txid] = tx;
    
    // Update pending state.
    address_states_[tx.sender].pending_deduction = total_required_with_pending;
    address_states_[tx.sender].highest_nonce = tx.nonce;

    return TxRejectReason::NONE;
}

std::vector<core::Transaction> Mempool::get_pending_transactions(size_t max_count) const {
    std::lock_guard<std::mutex> lock(mtx_);

    std::vector<core::Transaction> sorted_txs;
    sorted_txs.reserve(pending_txs_.size());

    for (const auto& [txid, tx] : pending_txs_) {
        sorted_txs.push_back(tx);
    }

    // --- Dharma Fair Ordering (FCFS Anti-MEV Sequencing) ---
    // Strict First-Come First-Served ordering by high-precision arrival timestamp.
    // Removes Priority Gas Auctions (PGA) to eliminate front-running and sandwiching.
    std::sort(sorted_txs.begin(), sorted_txs.end(), [](const core::Transaction& a, const core::Transaction& b) {
        if (a.arrival_timestamp_ns != b.arrival_timestamp_ns) {
            return a.arrival_timestamp_ns < b.arrival_timestamp_ns; // FCFS Arrival Order
        }
        if (a.nonce != b.nonce) {
            return a.nonce < b.nonce;
        }
        return a.txid() < b.txid();
    });

    if (sorted_txs.size() > max_count) {
        sorted_txs.resize(max_count);
    }

    return sorted_txs;
}

void Mempool::remove_transactions(const std::vector<core::Transaction>& txs) {
    std::lock_guard<std::mutex> lock(mtx_);

    for (const auto& tx : txs) {
        std::string txid = tx.txid();
        if (pending_txs_.erase(txid)) {
            // Recalculate address states from scratch to be safe.
            // In a highly optimized version, we'd decrement pending state.
        }
    }

    // Rebuild address states from remaining txs.
    address_states_.clear();
    for (const auto& [txid, tx] : pending_txs_) {
        address_states_[tx.sender].pending_deduction += (tx.amount + tx.fee);
        if (tx.nonce > address_states_[tx.sender].highest_nonce) {
            address_states_[tx.sender].highest_nonce = tx.nonce;
        }
    }
}

bool Mempool::has_transaction(const std::string& txid) const {
    std::lock_guard<std::mutex> lock(mtx_);
    return pending_txs_.find(txid) != pending_txs_.end();
}

size_t Mempool::size() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return pending_txs_.size();
}

void Mempool::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    pending_txs_.clear();
    address_states_.clear();
}

uint64_t Mempool::get_expected_nonce(const std::string& address, const state::StateDB& state) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = address_states_.find(address);
    if (it != address_states_.end()) {
        return it->second.highest_nonce + 1;
    }
    state::Account acc = state.get_account(address);
    return acc.nonce;
}

} // namespace mempool
} // namespace kasturisundari
