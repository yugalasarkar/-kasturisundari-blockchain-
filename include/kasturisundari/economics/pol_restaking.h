// Kasturisundari Chain — $POL Dual-Restaking Vault Engine
// Institutional dual-staking module enabling $POL (Polygon Token) restaking alongside $NILA native token.

#pragma once

#include "kasturisundari/crypto/sha256.h"
#include "kasturisundari/state/state_db.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace kasturisundari {
namespace economics {

/// Represents a Validator's Staking Record
struct ValidatorStakeRecord {
    std::string validator_address;
    uint64_t    staked_nila = 0;   // Native NILA stake
    uint64_t    staked_pol = 0;    // Restaked POL (Polygon) token
    uint64_t    pol_exchange_rate = 1; // Exchange rate multiplier for voting weight
    uint64_t    reward_accumulated_nila = 0;
    uint64_t    reward_accumulated_pol = 0;
    bool        is_slashed = false;

    /// Compute effective validator forging/sequencing weight
    uint64_t get_effective_weight() const {
        if (is_slashed) return 0;
        return staked_nila + (staked_pol * pol_exchange_rate);
    }
};

class PolRestakingVault {
public:
    PolRestakingVault(state::StateDB& state);

    /// Deposit & stake $POL for a validator
    bool stake_pol(const std::string& validator_address, uint64_t pol_amount);

    /// Deposit & stake native $NILA for a validator
    bool stake_nila(const std::string& validator_address, uint64_t nila_amount);

    /// Slash validator stake for double-signing or MEV reordering attempt
    bool slash_validator(const std::string& validator_address, double slash_percentage = 0.50);

    /// Distribute transaction fees to POL restakers
    void distribute_fee_rewards(uint64_t total_fee);

    /// Get validator staking telemetry
    ValidatorStakeRecord get_validator_stake(const std::string& validator_address) const;

    /// Get total POL restaked in vault across all validators
    uint64_t get_total_pol_restaked() const { return total_pol_restaked_; }

    /// Get total NILA staked
    uint64_t get_total_nila_staked() const { return total_nila_staked_; }

private:
    state::StateDB& state_;
    mutable std::mutex mtx_;

    uint64_t total_pol_restaked_ = 0;
    uint64_t total_nila_staked_ = 0;

    std::unordered_map<std::string, ValidatorStakeRecord> stakes_;
};

} // namespace economics
} // namespace kasturisundari
