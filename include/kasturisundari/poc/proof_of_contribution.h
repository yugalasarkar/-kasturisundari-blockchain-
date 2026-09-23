// Kasturisundari Chain — Proof of Contribution (PoC) Engine
// Handles the core economic mechanism of the chain: distributing NSK
// to developers who submit meaningful code contributions.

#pragma once

#include "kasturisundari/core/transaction.h"
#include "kasturisundari/state/state_db.h"
#include "kasturisundari/crypto/sha256.h"

#include <string>
#include <vector>

namespace kasturisundari {
namespace poc {

/// A proposal submitted by a developer containing the hash of their code commit.
struct ContributionProposal {
    std::string developer_address;
    crypto::Hash256 commit_hash;   // Hash of the PR or commit
    std::string description;       // Brief description of the feature
    uint64_t timestamp;

    /// Serialize for storage or hashing.
    std::vector<uint8_t> serialize() const;

    /// Deserialize from transaction data.
    static ContributionProposal deserialize(const std::vector<uint8_t>& data);
};

/// An approval of a contribution by the network (validator/founder).
/// Releases the reward from the Reward Pool to the developer.
struct ContributionApproval {
    crypto::Hash256 proposal_txid; // The TXID of the ContributionProposal
    uint64_t reward_amount;        // Amount calculated by the diminishing formula

    /// Serialize.
    std::vector<uint8_t> serialize() const;

    /// Deserialize.
    static ContributionApproval deserialize(const std::vector<uint8_t>& data);
};

/// Validates and applies a Contribution Approval transaction to the state.
/// This is the ONLY way NSK leaves the Reward Pool.
bool apply_contribution_reward(
    const core::Transaction& approval_tx,
    state::StateDB& state,
    uint64_t current_chain_height
);

} // namespace poc
} // namespace kasturisundari
