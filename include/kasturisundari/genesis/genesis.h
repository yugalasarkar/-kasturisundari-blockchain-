// Kasturisundari Chain — Genesis Block Builder
// Creates the very first block of the chain with the initial token distribution.

#pragma once

#include "kasturisundari/core/block.h"
#include "kasturisundari/state/state_db.h"

#include <string>

namespace kasturisundari {
namespace genesis {

/// Configuration for the genesis block.
struct GenesisConfig {
    std::string founder_address;    // Receives 1,000,000 NSK
    std::string secondary_address;  // Receives 1,000,000 NSK
    std::string reward_pool_address; // Holds 19,000,000 NSK (locked)
    uint64_t    timestamp;          // Genesis block timestamp
};

/// Build the genesis block from the given configuration.
/// This creates special transactions that allocate the initial supply.
core::Block build_genesis_block(const GenesisConfig& config);

/// Apply the genesis block to the state database.
/// Sets the initial balances for founder, secondary, and reward pool.
bool apply_genesis_to_state(const core::Block& genesis, state::StateDB& db);

/// Validate that a block is a valid genesis block.
bool is_valid_genesis(const core::Block& block);

} // namespace genesis
} // namespace kasturisundari
