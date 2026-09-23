// Kasturisundari Chain — $POL Dual-Restaking Vault Engine Implementation

#include "kasturisundari/economics/pol_restaking.h"
#include <iostream>
#include <algorithm>

namespace kasturisundari {
namespace economics {

PolRestakingVault::PolRestakingVault(state::StateDB& state) : state_(state) {}

bool PolRestakingVault::stake_pol(const std::string& validator_address, uint64_t pol_amount) {
    std::lock_guard<std::mutex> lock(mtx_);

    if (pol_amount == 0) return false;

    // Verify validator account balance in StateDB
    uint64_t bal = state_.get_balance(validator_address);
    if (bal < pol_amount) {
        std::cerr << "[POL Restaking Vault] Stake failed: Insufficient balance for validator=" << validator_address << "\n";
        return false;
    }

    // Deduct balance from validator for vault locking
    state::Account acc = state_.get_account(validator_address);
    acc.balance -= pol_amount;
    state_.put_account(validator_address, acc);

    // Record POL stake
    auto& rec = stakes_[validator_address];
    rec.validator_address = validator_address;
    rec.staked_pol += pol_amount;
    total_pol_restaked_ += pol_amount;

    std::cout << "[POL Restaking Vault] Validator " << validator_address 
              << " restaked " << pol_amount << " POL. Total POL Restaked: " << total_pol_restaked_ 
              << " | Effective Weight: " << rec.get_effective_weight() << "\n";

    return true;
}

bool PolRestakingVault::stake_nila(const std::string& validator_address, uint64_t nila_amount) {
    std::lock_guard<std::mutex> lock(mtx_);

    if (nila_amount == 0) return false;

    uint64_t bal = state_.get_balance(validator_address);
    if (bal < nila_amount) {
        return false;
    }

    state::Account acc = state_.get_account(validator_address);
    acc.balance -= nila_amount;
    state_.put_account(validator_address, acc);

    auto& rec = stakes_[validator_address];
    rec.validator_address = validator_address;
    rec.staked_nila += nila_amount;
    total_nila_staked_ += nila_amount;

    return true;
}

bool PolRestakingVault::slash_validator(const std::string& validator_address, double slash_percentage) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = stakes_.find(validator_address);
    if (it == stakes_.end() || it->second.is_slashed) return false;

    uint64_t slashed_pol = static_cast<uint64_t>(it->second.staked_pol * slash_percentage);
    uint64_t slashed_nila = static_cast<uint64_t>(it->second.staked_nila * slash_percentage);

    it->second.staked_pol -= slashed_pol;
    it->second.staked_nila -= slashed_nila;
    it->second.is_slashed = true;

    total_pol_restaked_ -= slashed_pol;
    total_nila_staked_ -= slashed_nila;

    std::cout << "[POL Restaking Vault] SLASHED validator " << validator_address 
              << " (Slashed POL: " << slashed_pol << ", Slashed NILA: " << slashed_nila << ")\n";

    return true;
}

void PolRestakingVault::distribute_fee_rewards(uint64_t total_fee) {
    std::lock_guard<std::mutex> lock(mtx_);

    if (total_fee == 0 || stakes_.empty()) return;

    // Calculate total effective weight across all non-slashed validators
    uint64_t total_weight = 0;
    for (const auto& [addr, rec] : stakes_) {
        if (!rec.is_slashed) total_weight += rec.get_effective_weight();
    }

    if (total_weight == 0) return;

    // Distribute rewards proportional to POL + NILA effective weight
    for (auto& [addr, rec] : stakes_) {
        if (rec.is_slashed) continue;
        uint64_t share = (total_fee * rec.get_effective_weight()) / total_weight;
        rec.reward_accumulated_pol += share;
    }
}

ValidatorStakeRecord PolRestakingVault::get_validator_stake(const std::string& validator_address) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = stakes_.find(validator_address);
    if (it != stakes_.end()) return it->second;
    return ValidatorStakeRecord{validator_address, 0, 0, 1, 0, 0, false};
}

} // namespace economics
} // namespace kasturisundari
