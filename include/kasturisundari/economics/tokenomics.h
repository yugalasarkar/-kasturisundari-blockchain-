// Kasturisundari Chain — Tokenomics Engine
// Enforces the economic rules of the network:
//   - 21,000,000 NSK hard cap
//   - Fee recycling back to the Reward Pool
//   - Diminishing contribution rewards over time
//
// All amounts are in the smallest unit: 1 NSK = 100,000,000 units (8 decimals).

#pragma once

#include <cstdint>
#include <string>

namespace kasturisundari {
namespace economics {

// ─── Constants ──────────────────────────────────────────────────────

/// 1 NSK = 100,000,000 smallest units (8 decimal places).
constexpr uint64_t UNITS_PER_NILA = 100'000'000ULL;

/// Maximum supply: 21,000,000 NSK in smallest units.
constexpr uint64_t MAX_SUPPLY = 21'000'000ULL * UNITS_PER_NILA;

/// Founder allocation: 1,000,000 NSK.
constexpr uint64_t FOUNDER_ALLOCATION = 1'000'000ULL * UNITS_PER_NILA;

/// Secondary wallet allocation: 1,000,000 NSK.
constexpr uint64_t SECONDARY_ALLOCATION = 1'000'000ULL * UNITS_PER_NILA;

/// Initial reward pool: 19,000,000 NSK (locked, released via contributions).
constexpr uint64_t INITIAL_REWARD_POOL = 19'000'000ULL * UNITS_PER_NILA;

/// Minimum transaction fee: 0 NSK.
constexpr uint64_t MIN_TRANSACTION_FEE = 0ULL;

static constexpr const char* TICKER = "NILA";
static constexpr const char* CURRENCY_NAME = "Nilashyam";

/// EVM Chain ID (unique for KasturiChain)
constexpr uint64_t EVM_CHAIN_ID = 108108ULL;

// ─── Reward Pool Address ────────────────────────────────────────────

/// The special address that holds the Reward Pool.
/// This is a deterministic address derived from a known seed,
/// not controlled by any private key.
extern const std::string REWARD_POOL_ADDRESS;

/// The founder wallet address (set during genesis).
extern const std::string FOUNDER_ADDRESS;

/// The secondary wallet address (set during genesis).
extern const std::string SECONDARY_ADDRESS;

// ─── Tokenomics Functions ───────────────────────────────────────────

/// Check if minting `amount` would exceed the 21M hard cap.
/// total_circulating: current total across all accounts + reward pool.
bool would_exceed_cap(uint64_t total_circulating, uint64_t amount);

/// Compute the contribution reward based on how much is left in the pool.
/// Uses a diminishing curve: as the pool shrinks, rewards get smaller.
///
/// Formula: reward = base_reward * (pool_remaining / INITIAL_REWARD_POOL)
///
/// This means:
///   - When pool is full (19M), reward = base_reward (maximum)
///   - When pool is half (9.5M), reward = base_reward / 2
///   - When pool is 1%, reward = base_reward / 100
///   - As NSK value rises, even tiny rewards become valuable
///
/// base_reward: the maximum possible reward for a contribution (in smallest units).
/// pool_remaining: current NSK left in the reward pool.
uint64_t compute_contribution_reward(uint64_t base_reward,
                                     uint64_t pool_remaining);

/// Compute an era-based base reward.
/// Halves every `era_blocks` blocks (similar to Bitcoin halving).
///   era 0: initial_base_reward
///   era 1: initial_base_reward / 2
///   era 2: initial_base_reward / 4
///   ...
uint64_t compute_era_base_reward(uint64_t initial_base_reward,
                                 uint64_t current_height,
                                 uint64_t era_blocks);

/// The initial base reward for contributions: 100 NSK per contribution.
constexpr uint64_t INITIAL_BASE_REWARD = 100ULL * UNITS_PER_NILA;

/// Number of blocks per era (halving interval).
constexpr uint64_t ERA_BLOCK_INTERVAL = 210'000ULL;

/// Validate that the total supply invariant holds:
///   founder + secondary + reward_pool + all_other_balances == MAX_SUPPLY
bool validate_supply_invariant(uint64_t founder_balance,
                               uint64_t secondary_balance,
                               uint64_t reward_pool_balance,
                               uint64_t other_balances_sum);

/// Convert smallest units to a human-readable NSK string (e.g., "1.50000000").
std::string format_nila(uint64_t amount);

/// Convert a NSK string (e.g., "1.5") to smallest units.
uint64_t parse_nsk(const std::string& nsk_str);

} // namespace economics
} // namespace kasturisundari
