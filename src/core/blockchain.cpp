// Kasturisundari Chain — Blockchain Manager Implementation

#include "kasturisundari/core/blockchain.h"

#include <algorithm>
#include <iostream>

namespace kasturisundari {
namespace core {

const char* add_block_result_to_string(AddBlockResult result) {
    switch (result) {
        case AddBlockResult::SUCCESS:              return "Success";
        case AddBlockResult::INVALID_PREVIOUS_HASH: return "InvalidPreviousHash";
        case AddBlockResult::INVALID_MERKLE_ROOT:   return "InvalidMerkleRoot";
        case AddBlockResult::INVALID_HEIGHT:         return "InvalidHeight";
        case AddBlockResult::INVALID_TIMESTAMP:      return "InvalidTimestamp";
        case AddBlockResult::INVALID_INTERNAL:       return "InvalidInternal";
        case AddBlockResult::DUPLICATE_BLOCK:        return "DuplicateBlock";
        default:                                     return "Unknown";
    }
}

Blockchain::Blockchain() = default;

void Blockchain::initialize_genesis(const Block& genesis) {
    chain_.clear();
    hash_index_.clear();

    chain_.push_back(genesis);
    hash_index_[genesis.block_id()] = 0;
}

bool Blockchain::validate_block(const Block& block) const {
    // Internal consistency check.
    if (!block.validate_internal())
        return false;

    // If this is genesis (height 0), previous hash must be all zeros.
    if (block.header.height == 0) {
        crypto::Hash256 zeros{};
        return block.header.previous_hash == zeros;
    }

    // Otherwise, previous hash must match the tip.
    if (chain_.empty())
        return false;

    const auto& tip = chain_.back();
    crypto::Hash256 tip_hash = tip.compute_hash();

    if (block.header.previous_hash != tip_hash)
        return false;

    // Height must be exactly tip + 1.
    if (block.header.height != tip.header.height + 1)
        return false;

    // Timestamp must not be before the previous block.
    if (block.header.timestamp < tip.header.timestamp)
        return false;

    return true;
}

AddBlockResult Blockchain::add_block(const Block& block) {
    // Check for duplicate.
    std::string bid = block.block_id();
    if (hash_index_.count(bid))
        return AddBlockResult::DUPLICATE_BLOCK;

    // Internal consistency.
    if (!block.validate_internal())
        return AddBlockResult::INVALID_INTERNAL;

    if (!chain_.empty()) {
        const auto& tip = chain_.back();
        crypto::Hash256 tip_hash = tip.compute_hash();

        if (block.header.previous_hash != tip_hash)
            return AddBlockResult::INVALID_PREVIOUS_HASH;

        if (block.header.height != tip.header.height + 1)
            return AddBlockResult::INVALID_HEIGHT;

        if (block.header.timestamp < tip.header.timestamp)
            return AddBlockResult::INVALID_TIMESTAMP;
    }

    // Verify merkle root.
    crypto::Hash256 expected_root = block.compute_merkle_root();
    if (expected_root != block.header.merkle_root)
        return AddBlockResult::INVALID_MERKLE_ROOT;

    // All checks passed — add the block.
    size_t index = chain_.size();
    chain_.push_back(block);
    hash_index_[bid] = index;

    return AddBlockResult::SUCCESS;
}

const Block& Blockchain::get_latest_block() const {
    return chain_.back();
}

std::optional<std::reference_wrapper<const Block>>
Blockchain::get_block_at(uint64_t height) const {
    if (height >= chain_.size()) return std::nullopt;
    return std::cref(chain_[height]);
}

std::optional<std::reference_wrapper<const Block>>
Blockchain::get_block_by_hash(const crypto::Hash256& hash) const {
    std::string hex = crypto::hash_to_hex(hash);
    auto it = hash_index_.find(hex);
    if (it == hash_index_.end()) return std::nullopt;
    return std::cref(chain_[it->second]);
}

uint64_t Blockchain::get_height() const {
    if (chain_.empty()) return 0;
    return chain_.back().header.height;
}

size_t Blockchain::block_count() const {
    return chain_.size();
}

bool Blockchain::validate_full_chain() const {
    if (chain_.empty()) return true;

    // Validate genesis.
    crypto::Hash256 zeros{};
    if (chain_[0].header.previous_hash != zeros)
        return false;
    if (!chain_[0].validate_internal())
        return false;

    // Validate each subsequent block.
    for (size_t i = 1; i < chain_.size(); ++i) {
        const auto& prev = chain_[i - 1];
        const auto& curr = chain_[i];

        if (!curr.validate_internal())
            return false;

        if (curr.header.previous_hash != prev.compute_hash())
            return false;

        if (curr.header.height != prev.header.height + 1)
            return false;

        if (curr.header.timestamp < prev.header.timestamp)
            return false;

        crypto::Hash256 expected_root = curr.compute_merkle_root();
        if (expected_root != curr.header.merkle_root)
            return false;
    }

    return true;
}

uint64_t Blockchain::get_total_recycled_fees() const {
    uint64_t total = 0;
    for (const auto& block : chain_) {
        total += block.header.total_fees;
    }
    return total;
}

crypto::Hash256 Blockchain::get_tip_hash() const {
    if (chain_.empty()) return crypto::Hash256{};
    return chain_.back().compute_hash();
}

bool Blockchain::contains_transaction(const crypto::Hash256& tx_hash) const {
    for (const auto& block : chain_) {
        for (const auto& tx : block.transactions) {
            if (tx.compute_hash() == tx_hash)
                return true;
        }
    }
    return false;
}

Blockchain::StateSnapshotProof Blockchain::generate_state_proof() const {
    StateSnapshotProof proof;
    if (chain_.empty()) return proof;

    const Block& tip = chain_.back();
    proof.finalized_height = tip.header.height;
    proof.finalized_block_hash = tip.compute_hash();

    std::vector<uint8_t> acc_buf;
    for (const auto& b : chain_) {
        auto h = b.compute_hash();
        proof.header_hashes.push_back(h);
        acc_buf.insert(acc_buf.end(), h.begin(), h.end());
    }

    proof.accumulator_root = crypto::double_sha256(acc_buf);
    proof.state_root = tip.header.merkle_root;
    return proof;
}

bool Blockchain::bootstrap_from_state_proof(const StateSnapshotProof& proof, const Block& finalized_tip) {
    if (finalized_tip.header.height != proof.finalized_height) return false;
    if (finalized_tip.compute_hash() != proof.finalized_block_hash) return false;

    chain_.clear();
    hash_index_.clear();

    chain_.push_back(finalized_tip);
    hash_index_[finalized_tip.block_id()] = 0;

    light_sovereign_mode_ = true;
    current_state_proof_ = proof;

    std::cout << "[Blockchain] Bootstrapped light sovereign state at height #" 
              << proof.finalized_height << " [Tip: " << finalized_tip.block_id() << "]\n";
    return true;
}

} // namespace core
} // namespace kasturisundari
