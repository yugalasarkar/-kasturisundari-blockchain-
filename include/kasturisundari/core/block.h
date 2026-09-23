// Kasturisundari Chain — Block Structure
// A block is a container of transactions sealed by a cryptographic hash.

#pragma once

#include "kasturisundari/core/transaction.h"
#include "kasturisundari/crypto/sha256.h"

#include <cstdint>
#include <string>
#include <vector>

namespace kasturisundari {
namespace core {

/// The block header — lightweight summary that chains blocks together.
struct BlockHeader {
    uint32_t          version;            // Protocol version
    uint64_t          height;             // Block number (0 = genesis)
    crypto::Hash256   previous_hash;      // Hash of the previous block header
    crypto::Hash256   merkle_root;        // Merkle root of all transactions
    crypto::Hash256   attestation_root;   // Merkle root of text attestations in this block
    uint64_t          timestamp;          // Unix timestamp
    uint32_t          tx_count;           // Number of transactions in the block
    uint64_t          total_fees;         // Sum of all fees in this block (recycled to pool)

    /// Compute the SHA-256 hash of this block header.
    crypto::Hash256 compute_hash() const;

    /// Serialize the block header to binary.
    std::vector<uint8_t> serialize() const;

    /// Deserialize a block header from binary.
    static BlockHeader deserialize(const std::vector<uint8_t>& data);
};

/// A complete block: header + body (transactions).
struct Block {
    BlockHeader                 header;
    std::vector<Transaction>    transactions;

    /// Build the Merkle root from the block's transactions.
    crypto::Hash256 compute_merkle_root() const;

    /// Compute the block hash (hash of the header).
    crypto::Hash256 compute_hash() const;

    /// Validate internal consistency:
    ///   - Merkle root matches transactions
    ///   - tx_count matches
    ///   - total_fees matches sum of tx fees
    bool validate_internal() const;

    /// Serialize the entire block (header + all transactions).
    std::vector<uint8_t> serialize() const;

    /// Deserialize a complete block from binary.
    static Block deserialize(const std::vector<uint8_t>& data);

    /// Get the block hash as a hex string.
    std::string block_id() const;

    /// Get the block size in bytes.
    size_t size_bytes() const;
};

/// Compute the Merkle root from a vector of transaction hashes.
crypto::Hash256 compute_tx_merkle_root(const std::vector<crypto::Hash256>& tx_hashes);

} // namespace core
} // namespace kasturisundari
