// Kasturisundari Chain — Vedic Sabhā (Governance Engine)
// Handles on-chain voting for network upgrades and protocol parameters.
// Only developers who hold PoC NSK can vote (Proof of Stake model).

#pragma once

#include "kasturisundari/crypto/sha256.h"
#include "kasturisundari/state/state_db.h"
#include <string>
#include <vector>

namespace kasturisundari {
namespace governance {

class Sabha {
public:
    Sabha(state::StateDB& state_db);

    /// Submit a new governance proposal (e.g., "UPGRADE_TO_VERSION_2").
    /// Requires burning a small fee to prevent spam.
    bool submit_proposal(const crypto::Hash256& proposal_hash, const std::string& proposer_address);

    /// Cast a vote for a proposal.
    /// The weight of the vote equals the NSK balance of the voter.
    bool cast_vote(const crypto::Hash256& proposal_hash, const std::string& voter_address, bool approve);

    /// Tally the votes for a proposal.
    /// Returns {yes_weight, no_weight}.
    std::pair<uint64_t, uint64_t> get_tally(const crypto::Hash256& proposal_hash) const;

    /// Check if a proposal has passed (yes > no AND quorum reached).
    bool is_proposal_passed(const crypto::Hash256& proposal_hash) const;

    /// Get the quorum threshold (51% of total circulating supply).
    uint64_t get_quorum_threshold() const;

private:
    state::StateDB& state_;
};

} // namespace governance
} // namespace kasturisundari
