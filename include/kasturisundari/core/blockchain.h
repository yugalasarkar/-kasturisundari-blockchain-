// Kasturisundari Chain — Blockchain Manager
// Manages the chain of blocks, validates new blocks, and maintains integrity.

#pragma once

#include "kasturisundari/core/block.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace kasturisundari {
namespace core {

/// Result of attempting to add a block to the chain.
enum class AddBlockResult {
    SUCCESS,
    INVALID_PREVIOUS_HASH,
    INVALID_MERKLE_ROOT,
    INVALID_HEIGHT,
    INVALID_TIMESTAMP,
    INVALID_INTERNAL,
    DUPLICATE_BLOCK,
};

/// Convert AddBlockResult to a human-readable string.
const char* add_block_result_to_string(AddBlockResult result);

/// The blockchain: an ordered, validated chain of blocks.
class Blockchain {
public:
    Blockchain();

    /// Initialize the chain with the genesis block.
    void initialize_genesis(const Block& genesis);

    /// Attempt to add a new block to the chain.
    /// Validates the block before accepting it.
    AddBlockResult add_block(const Block& block);

    /// Get the latest (tip) block.
    const Block& get_latest_block() const;

    /// Get a block by its height.
    std::optional<std::reference_wrapper<const Block>> get_block_at(uint64_t height) const;

    /// Get a block by its hash.
    std::optional<std::reference_wrapper<const Block>> get_block_by_hash(
        const crypto::Hash256& hash) const;

    /// Get the current chain height (number of blocks - 1).
    uint64_t get_height() const;

    /// Get the total number of blocks.
    size_t block_count() const;

    /// Validate the entire chain from genesis to tip.
    /// Returns true only if every block is valid and properly linked.
    bool validate_full_chain() const;

    /// Get the total fees collected across all blocks (all recycled to pool).
    uint64_t get_total_recycled_fees() const;

    /// Get the hash of the chain tip.
    crypto::Hash256 get_tip_hash() const;

    /// Succinct State Snapshot Proof for lightweight pruning & fast sync
    struct StateSnapshotProof {
        uint64_t finalized_height = 0;
        crypto::Hash256 finalized_block_hash{};
        crypto::Hash256 state_root{};
        crypto::Hash256 accumulator_root{};
        std::vector<crypto::Hash256> header_hashes;
    };

    /// Generate succinct state snapshot proof for historical pruning.
    StateSnapshotProof generate_state_proof() const;

    /// Fast bootstrap a light sovereign node from a state snapshot proof.
    bool bootstrap_from_state_proof(const StateSnapshotProof& proof, const Block& finalized_tip);

    /// Check if node is operating in pruned light sovereign mode.
    bool is_light_sovereign() const { return light_sovereign_mode_; }
    void set_light_sovereign(bool enable) { light_sovereign_mode_ = enable; }

    /// Check if the chain contains a specific transaction by its hash.
    bool contains_transaction(const crypto::Hash256& tx_hash) const;

private:
    bool light_sovereign_mode_ = false;
    StateSnapshotProof current_state_proof_{};
    std::vector<Block> chain_;
    std::unordered_map<std::string, size_t> hash_index_;  // block_hash_hex -> index

    bool validate_block(const Block& block) const;
};

} // namespace core
} // namespace kasturisundari
