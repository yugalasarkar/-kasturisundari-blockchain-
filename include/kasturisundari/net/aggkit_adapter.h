// Kasturisundari Chain — AggKit LxLy Bridge Adapter & Pessimistic Proof Engine
// Interoperability bridge connecting KasturiChain state roots to Polygon AggLayer (LxLy Unified Bridge).

#pragma once

#include "kasturisundari/crypto/sha256.h"
#include "kasturisundari/state/state_db.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <mutex>

namespace kasturisundari {
namespace net {

/// Cross-Chain Bridge Message Frame for bridgeAndCall()
struct LxLyBridgeMessage {
    uint32_t origin_network;
    std::string origin_address;
    uint32_t destination_network;
    std::string destination_address;
    uint64_t amount;
    std::vector<uint8_t> payload;
    uint64_t deposit_count;

    crypto::Hash256 compute_leaf_hash() const;
};

/// Pessimistic Proof structure for AggLayer verification
struct AggKitPessimisticProof {
    uint32_t network_id = 108108; // KasturiChain EIP-155 Chain ID
    crypto::Hash256 local_exit_root{};
    crypto::Hash256 global_exit_root{};
    uint64_t total_deposits = 0;
    uint64_t total_withdrawals = 0;
    std::vector<crypto::Hash256> exit_tree_proof;

    /// Validates mathematical invariant: Total Withdrawals <= Total Deposits
    bool verify_pessimistic_invariant() const {
        return total_withdrawals <= total_deposits;
    }
};

class AggKitAdapter {
public:
    AggKitAdapter(state::StateDB& state);

    /// Execute a cross-chain bridgeAndCall deposit to AggLayer Unified Bridge
    bool bridge_and_call(uint32_t dest_network,
                         const std::string& sender,
                         const std::string& recipient,
                         uint64_t amount,
                         const std::vector<uint8_t>& payload,
                         crypto::Hash256& out_leaf_hash);

    /// Generate Pessimistic Proof for local chain state
    AggKitPessimisticProof generate_pessimistic_proof() const;

    /// Submit & verify an incoming Pessimistic Proof from AggLayer
    bool submit_pessimistic_proof(const AggKitPessimisticProof& proof);

    /// Get current Global Exit Root
    crypto::Hash256 get_global_exit_root() const;

    /// Get total local deposit count
    uint64_t get_deposit_count() const { return deposit_count_; }

private:
    state::StateDB& state_;
    mutable std::mutex mtx_;

    uint64_t deposit_count_ = 0;
    uint64_t total_deposited_amount_ = 0;
    uint64_t total_withdrawn_amount_ = 0;

    std::vector<crypto::Hash256> exit_leaves_;
    crypto::Hash256 current_local_exit_root_{};
    crypto::Hash256 current_global_exit_root_{};

    crypto::Hash256 compute_merkle_tree_root(const std::vector<crypto::Hash256>& leaves) const;
};

} // namespace net
} // namespace kasturisundari
