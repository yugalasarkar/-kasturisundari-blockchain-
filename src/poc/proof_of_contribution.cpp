// Kasturisundari Chain — Proof of Contribution Implementation

#include "kasturisundari/poc/proof_of_contribution.h"
#include "kasturisundari/poc/poc_forger.h"
#include "kasturisundari/economics/tokenomics.h"

#include <cstring>

namespace kasturisundari {
namespace poc {

namespace {

void write_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val >> 24));
    buf.push_back(static_cast<uint8_t>(val >> 16));
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val));
}

void write_u64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 7; i >= 0; --i)
        buf.push_back(static_cast<uint8_t>(val >> (i * 8)));
}

void write_string(std::vector<uint8_t>& buf, const std::string& s) {
    write_u32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

uint32_t read_u32(const uint8_t*& p) {
    uint32_t v = (static_cast<uint32_t>(p[0]) << 24)
               | (static_cast<uint32_t>(p[1]) << 16)
               | (static_cast<uint32_t>(p[2]) << 8) | p[3];
    p += 4; return v;
}

uint64_t read_u64(const uint8_t*& p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    p += 8; return v;
}

std::string read_string(const uint8_t*& p) {
    uint32_t len = read_u32(p);
    std::string s(reinterpret_cast<const char*>(p), len);
    p += len; return s;
}

} // anonymous namespace

// ─── ContributionProposal ───────────────────────────────────────────

std::vector<uint8_t> ContributionProposal::serialize() const {
    std::vector<uint8_t> buf;
    write_string(buf, developer_address);
    buf.insert(buf.end(), commit_hash.begin(), commit_hash.end());
    write_string(buf, description);
    write_u64(buf, timestamp);
    return buf;
}

ContributionProposal ContributionProposal::deserialize(const std::vector<uint8_t>& data) {
    ContributionProposal p;
    if (data.empty()) return p;
    
    const uint8_t* ptr = data.data();
    p.developer_address = read_string(ptr);
    
    std::memcpy(p.commit_hash.data(), ptr, 32);
    ptr += 32;
    
    p.description = read_string(ptr);
    p.timestamp = read_u64(ptr);
    
    return p;
}

// ─── ContributionApproval ───────────────────────────────────────────

std::vector<uint8_t> ContributionApproval::serialize() const {
    std::vector<uint8_t> buf;
    buf.insert(buf.end(), proposal_txid.begin(), proposal_txid.end());
    write_u64(buf, reward_amount);
    return buf;
}

ContributionApproval ContributionApproval::deserialize(const std::vector<uint8_t>& data) {
    ContributionApproval a;
    if (data.empty()) return a;
    
    const uint8_t* ptr = data.data();
    std::memcpy(a.proposal_txid.data(), ptr, 32);
    ptr += 32;
    
    a.reward_amount = read_u64(ptr);
    
    return a;
}

// ─── apply_contribution_reward ──────────────────────────────────────

bool apply_contribution_reward(
    const core::Transaction& approval_tx,
    state::StateDB& state,
    uint64_t current_chain_height
) {
    // 1. Transaction must be of type CONTRIBUTION_APPROVAL
    if (approval_tx.type != core::TxType::CONTRIBUTION_APPROVAL) {
        return false;
    }

    // 2. Parse the payload
    ContributionApproval approval = ContributionApproval::deserialize(approval_tx.data);

    // 3. Sender must be the network administrator / founder for genesis era
    // In a fully mature state, this could be a multisig of validators.
    if (approval_tx.sender != economics::FOUNDER_ADDRESS) {
        return false;
    }

    // 4. Calculate the mathematically guaranteed reward
    uint64_t current_pool_balance = state.get_balance(economics::REWARD_POOL_ADDRESS);
    
    uint64_t current_base = economics::compute_era_base_reward(
        economics::INITIAL_BASE_REWARD,
        current_chain_height,
        economics::ERA_BLOCK_INTERVAL
    );

    uint64_t expected_reward = economics::compute_contribution_reward(
        current_base,
        current_pool_balance
    );

    // 5. Validation: The amount claimed in the approval must match the exact mathematical formula
    if (approval.reward_amount != expected_reward) {
        return false;
    }

    // 6. Ensure reward pool has enough funds (it always should based on the formula, but safety first)
    if (current_pool_balance < expected_reward) {
        return false;
    }

    // 7. Execute the reward transfer
    state::Account pool_acc = state.get_account(economics::REWARD_POOL_ADDRESS);
    state::Account dev_acc = state.get_account(approval_tx.recipient);

    pool_acc.balance -= expected_reward;
    dev_acc.balance += expected_reward;

    if (!state.put_account(economics::REWARD_POOL_ADDRESS, pool_acc)) return false;
    if (!state.put_account(approval_tx.recipient, dev_acc)) return false;

    // To prevent a proposal from being approved twice, we store a metadata flag.
    std::string txid_hex = crypto::hash_to_hex(approval.proposal_txid);
    std::string meta_key = "poc_approved:" + txid_hex;
    
    // If it's already approved, reject.
    if (state.get_meta(meta_key).has_value()) {
        return false;
    }
    
    state.put_meta(meta_key, "1");

    // Increment the developer's PoC score for forging priority
    increment_poc_score(approval_tx.recipient, state);

    return true;
}

} // namespace poc
} // namespace kasturisundari
