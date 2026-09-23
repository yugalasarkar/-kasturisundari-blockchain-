// Kasturisundari Chain — Vedic Storage (P2P File Engine)
// Decentralized, chunk-based storage for preserving Vedic Audio and Texts.

#pragma once

#include "kasturisundari/crypto/sha256.h"
#include <vector>
#include <string>

namespace kasturisundari {
namespace storage {

class VedicStorage {
public:
    VedicStorage(const std::string& base_path = "./kasturi_data/storage");

    /// Initialize the storage directories.
    bool init();

    /// Store a raw chunk of data (Audio PCM or UTF-8 Text).
    /// The chunk is indexed by its SHA-256 hash.
    bool store_chunk(const std::vector<uint8_t>& data);

    /// Retrieve a chunk by its SHA-256 hash.
    /// Returns an empty vector if not found.
    std::vector<uint8_t> get_chunk(const crypto::Hash256& chunk_hash) const;

    /// Check if a chunk exists locally.
    bool has_chunk(const crypto::Hash256& chunk_hash) const;

private:
    std::string base_path_;
    
    // Convert Hash256 to a file path
    std::string get_chunk_path(const crypto::Hash256& hash) const;
};

} // namespace storage
} // namespace kasturisundari
