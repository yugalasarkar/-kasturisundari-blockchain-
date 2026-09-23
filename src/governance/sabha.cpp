// Kasturisundari Chain — Vedic Sabha Implementation

#include "kasturisundari/governance/sabha.h"
#include "kasturisundari/economics/tokenomics.h"
#include <iostream>

namespace kasturisundari {
namespace governance {

Sabha::Sabha(state::StateDB& state_db) : state_(state_db) {}

bool Sabha::submit_proposal(const crypto::Hash256& proposal_hash, const std::string& proposer_address) {
    std::string hash_hex = crypto::hash_to_hex(proposal_hash);
    std::string key = "gov_prop_" + hash_hex;
    
    // Check if proposal already exists
    if (state_.get_contract_state("SABHA_SYSTEM", key) != "0") {
        return false;
    }

    // Initialize proposal state (Format: proposer_address|yes_votes|no_votes)
    state_.set_contract_state("SABHA_SYSTEM", key, proposer_address + "|0|0");
    
    // Append to list of all proposals
    std::string list_key = "gov_proposals_list";
    std::string current_list = state_.get_contract_state("SABHA_SYSTEM", list_key);
    if (current_list == "0") current_list = "";
    if (!current_list.empty()) current_list += ",";
    current_list += hash_hex;
    state_.set_contract_state("SABHA_SYSTEM", list_key, current_list);
    
    return true;
}

bool Sabha::cast_vote(const crypto::Hash256& proposal_hash, const std::string& voter_address, bool approve) {
    std::string hash_hex = crypto::hash_to_hex(proposal_hash);
    std::string key = "gov_prop_" + hash_hex;
    
    std::string prop_state = state_.get_contract_state("SABHA_SYSTEM", key);
    if (prop_state == "0") return false; // Proposal doesn't exist

    // Prevent double voting
    std::string voter_key = "gov_vote_" + hash_hex + "_" + voter_address;
    if (state_.get_contract_state("SABHA_SYSTEM", voter_key) == "1") {
        return false; // Already voted
    }

    uint64_t voter_balance = state_.get_balance(voter_address);
    if (voter_balance == 0) return false; // Zero weight

    // Parse current tally
    size_t first_pipe = prop_state.find('|');
    size_t second_pipe = prop_state.find('|', first_pipe + 1);
    
    std::string proposer = prop_state.substr(0, first_pipe);
    uint64_t yes_votes = std::stoull(prop_state.substr(first_pipe + 1, second_pipe - first_pipe - 1));
    uint64_t no_votes = std::stoull(prop_state.substr(second_pipe + 1));

    if (approve) {
        yes_votes += voter_balance;
    } else {
        no_votes += voter_balance;
    }

    // Save updated state
    std::string new_state = proposer + "|" + std::to_string(yes_votes) + "|" + std::to_string(no_votes);
    state_.set_contract_state("SABHA_SYSTEM", key, new_state);
    
    // Mark voter as voted
    state_.set_contract_state("SABHA_SYSTEM", voter_key, "1");

    return true;
}

std::pair<uint64_t, uint64_t> Sabha::get_tally(const crypto::Hash256& proposal_hash) const {
    std::string hash_hex = crypto::hash_to_hex(proposal_hash);
    std::string key = "gov_prop_" + hash_hex;
    
    std::string prop_state = state_.get_contract_state("SABHA_SYSTEM", key);
    if (prop_state == "0") return {0, 0};

    size_t first_pipe = prop_state.find('|');
    size_t second_pipe = prop_state.find('|', first_pipe + 1);
    
    uint64_t yes_votes = std::stoull(prop_state.substr(first_pipe + 1, second_pipe - first_pipe - 1));
    uint64_t no_votes = std::stoull(prop_state.substr(second_pipe + 1));

    return {yes_votes, no_votes};
}

bool Sabha::is_proposal_passed(const crypto::Hash256& proposal_hash) const {
    auto [yes_votes, no_votes] = get_tally(proposal_hash);
    uint64_t total_votes = yes_votes + no_votes;
    
    // Must have quorum (51% of circulating supply participating)
    uint64_t quorum = get_quorum_threshold();
    if (total_votes < quorum) return false;
    
    // Yes must exceed No
    return yes_votes > no_votes;
}

uint64_t Sabha::get_quorum_threshold() const {
    // 51% of the total circulating supply (everything outside the Reward Pool)
    uint64_t pool_balance = state_.get_balance(economics::REWARD_POOL_ADDRESS);
    uint64_t circulating = economics::MAX_SUPPLY - pool_balance;
    return (circulating * 51) / 100;
}

} // namespace governance
} // namespace kasturisundari

