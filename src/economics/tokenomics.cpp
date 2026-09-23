// Kasturisundari Chain — Tokenomics Engine Implementation

#include "kasturisundari/economics/tokenomics.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace kasturisundari {
namespace economics {

// ─── Well-known addresses ───────────────────────────────────────────
// These are deterministic placeholder addresses.
// In production, the founder sets these during genesis configuration.

const std::string REWARD_POOL_ADDRESS = "KASTURISUNDARI_REWARD_POOL";
const std::string FOUNDER_ADDRESS     = "kasturiba37b624a4d1679593c649bffe616edd447911fd";
const std::string SECONDARY_ADDRESS   = "KASTURISUNDARI_SECONDARY";

// ─── Tokenomics Functions ───────────────────────────────────────────

bool would_exceed_cap(uint64_t total_circulating, uint64_t amount) {
    // Overflow check.
    if (amount > MAX_SUPPLY) return true;
    if (total_circulating > MAX_SUPPLY - amount) return true;
    return false;
}

uint64_t compute_contribution_reward(uint64_t base_reward,
                                     uint64_t pool_remaining) {
    if (pool_remaining == 0) return 0;
    if (base_reward == 0) return 0;

    // Diminishing reward: reward = base_reward * (pool_remaining / INITIAL_REWARD_POOL)
    // Use 128-bit arithmetic to avoid overflow.
    __uint128_t numerator = static_cast<__uint128_t>(base_reward) * pool_remaining;
    uint64_t reward = static_cast<uint64_t>(numerator / INITIAL_REWARD_POOL);

    // Ensure we never exceed what remains in the pool.
    if (reward > pool_remaining) reward = pool_remaining;

    // Ensure we always give at least 1 unit if pool is not empty and base > 0.
    if (reward == 0 && pool_remaining > 0) reward = 1;

    return reward;
}

uint64_t compute_era_base_reward(uint64_t initial_base_reward,
                                 uint64_t current_height,
                                 uint64_t era_blocks) {
    if (era_blocks == 0) return initial_base_reward;

    uint64_t era = current_height / era_blocks;

    // Cap at 64 halvings (effectively zero).
    if (era >= 64) return 0;

    return initial_base_reward >> era;  // Divide by 2^era
}

bool validate_supply_invariant(uint64_t founder_balance,
                               uint64_t secondary_balance,
                               uint64_t reward_pool_balance,
                               uint64_t other_balances_sum) {
    // The sum of ALL balances must always equal MAX_SUPPLY.
    // This is the fundamental economic invariant of Kasturisundari Chain.
    __uint128_t total = static_cast<__uint128_t>(founder_balance)
                      + secondary_balance
                      + reward_pool_balance
                      + other_balances_sum;

    return total == static_cast<__uint128_t>(MAX_SUPPLY);
}

std::string format_nila(uint64_t amount) {
    uint64_t whole = amount / UNITS_PER_NILA;
    uint64_t frac  = amount % UNITS_PER_NILA;

    std::ostringstream oss;
    oss << whole << "." << std::setfill('0') << std::setw(8) << frac;
    return oss.str();
}

uint64_t parse_nsk(const std::string& nsk_str) {
    size_t dot_pos = nsk_str.find('.');
    if (dot_pos == std::string::npos) {
        // No decimal point — treat as whole NSK.
        return std::stoull(nsk_str) * UNITS_PER_NILA;
    }

    uint64_t whole = std::stoull(nsk_str.substr(0, dot_pos));
    std::string frac_str = nsk_str.substr(dot_pos + 1);

    // Pad or truncate to 8 digits.
    while (frac_str.size() < 8) frac_str += '0';
    frac_str = frac_str.substr(0, 8);

    uint64_t frac = std::stoull(frac_str);
    return whole * UNITS_PER_NILA + frac;
}

} // namespace economics
} // namespace kasturisundari
