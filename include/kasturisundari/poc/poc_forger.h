// Kasturisundari Chain — PoC Forger Selection Engine
// Determines which node has the right to forge the next block
// based on Proof of Contribution scores.

#pragma once

#include "kasturisundari/state/state_db.h"
#include "kasturisundari/crypto/sha256.h"

#include <string>
#include <cstdint>

namespace kasturisundari {
namespace poc {

/// Retrieve the cumulative PoC score for a given address.
/// The score is the number of approved contributions stored in StateDB metadata.
uint64_t get_poc_score(const std::string& address, const state::StateDB& state);

/// Increment the PoC score for a given address after a successful contribution.
void increment_poc_score(const std::string& address, state::StateDB& state);

/// Compute the block forging interval for a node based on its PoC score.
/// Higher PoC scores result in shorter intervals (more frequent forging rights).
///
/// Formula:
///   interval = max(BASE_INTERVAL - (poc_score * SCORE_BONUS), MIN_INTERVAL)
///
/// This means:
///   - A node with 0 contributions forges every 30 seconds (slow)
///   - A node with 10 contributions forges every 10 seconds (fast)
///   - Minimum interval is capped at 5 seconds (prevents spam)
///
/// Returns the forging interval in seconds.
uint64_t compute_forging_interval(uint64_t poc_score);

/// Base forging interval for nodes with zero contributions (30 seconds).
constexpr uint64_t BASE_FORGING_INTERVAL = 1;

/// Bonus seconds subtracted per contribution.
constexpr uint64_t SCORE_BONUS_PER_CONTRIBUTION = 2;

/// Minimum forging interval (5 seconds) even for the highest contributors.
constexpr uint64_t MIN_FORGING_INTERVAL = 5;

} // namespace poc
} // namespace kasturisundari
