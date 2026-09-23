// Kasturisundari Chain — Block Implementation

#include "kasturisundari/core/block.h"
#include "kasturisundari/attestation/text_attestation.h"

#include <cstring>
#include <numeric>

namespace kasturisundari {
namespace core {

namespace {

void write_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val >> 24));
    buf.push_back(static_cast<uint8_t>(val >> 16));
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val));
}

void write_u64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 7; i >= 0; --i)
        buf.push_back(static_cast<uint8_t>(val >> (i * 8)));
}

uint32_t read_u32(const uint8_t*& p) {
    uint32_t v = (static_cast<uint32_t>(p[0]) << 24)
               | (static_cast<uint32_t>(p[1]) << 16)
               | (static_cast<uint32_t>(p[2]) << 8) | p[3];
    p += 4; return v;
}

uint64_t read_u64(const uint8_t*& p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    p += 8; return v;
}

} // anonymous namespace

// ─── Merkle Root ────────────────────────────────────────────────────

crypto::Hash256 compute_tx_merkle_root(
        const std::vector<crypto::Hash256>& tx_hashes) {
    if (tx_hashes.empty()) return crypto::Hash256{};
    if (tx_hashes.size() == 1) return tx_hashes[0];

    std::vector<crypto::Hash256> level = tx_hashes;

    while (level.size() > 1) {
        if (level.size() % 2 != 0)
            level.push_back(level.back());

        std::vector<crypto::Hash256> next;
        next.reserve(level.size() / 2);

        for (size_t i = 0; i < level.size(); i += 2) {
            std::vector<uint8_t> combined;
            combined.reserve(64);
            combined.insert(combined.end(),
                            level[i].begin(), level[i].end());
            combined.insert(combined.end(),
                            level[i + 1].begin(), level[i + 1].end());
            next.push_back(crypto::double_sha256(combined));
        }
        level = std::move(next);
    }
    return level[0];
}

// ─── BlockHeader ────────────────────────────────────────────────────

std::vector<uint8_t> BlockHeader::serialize() const {
    std::vector<uint8_t> buf;
    buf.reserve(128);

    write_u32(buf, version);
    write_u64(buf, height);
    buf.insert(buf.end(), previous_hash.begin(), previous_hash.end());
    buf.insert(buf.end(), merkle_root.begin(), merkle_root.end());
    buf.insert(buf.end(), attestation_root.begin(), attestation_root.end());
    write_u64(buf, timestamp);
    write_u32(buf, tx_count);
    write_u64(buf, total_fees);

    return buf;
}

crypto::Hash256 BlockHeader::compute_hash() const {
    auto buf = serialize();
    return crypto::double_sha256(buf);
}

BlockHeader BlockHeader::deserialize(const std::vector<uint8_t>& data) {
    BlockHeader h;
    const uint8_t* p = data.data();

    h.version = read_u32(p);
    h.height  = read_u64(p);
    std::memcpy(h.previous_hash.data(), p, 32); p += 32;
    std::memcpy(h.merkle_root.data(), p, 32);   p += 32;
    std::memcpy(h.attestation_root.data(), p, 32); p += 32;
    h.timestamp  = read_u64(p);
    h.tx_count   = read_u32(p);
    h.total_fees = read_u64(p);

    return h;
}

// ─── Block ──────────────────────────────────────────────────────────

crypto::Hash256 Block::compute_merkle_root() const {
    std::vector<crypto::Hash256> tx_hashes;
    tx_hashes.reserve(transactions.size());
    for (const auto& tx : transactions) {
        tx_hashes.push_back(tx.compute_hash());
    }
    return compute_tx_merkle_root(tx_hashes);
}

crypto::Hash256 Block::compute_hash() const {
    return header.compute_hash();
}

bool Block::validate_internal() const {
    // Check tx_count matches.
    if (header.tx_count != static_cast<uint32_t>(transactions.size()))
        return false;

    // Check merkle root matches.
    crypto::Hash256 expected_root = compute_merkle_root();
    if (expected_root != header.merkle_root)
        return false;

    // Check total_fees matches sum of tx fees.
    uint64_t fee_sum = 0;
    for (const auto& tx : transactions) {
        fee_sum += tx.fee;
    }
    if (fee_sum != header.total_fees)
        return false;

    return true;
}

std::vector<uint8_t> Block::serialize() const {
    // Header + serialized transactions.
    auto buf = header.serialize();

    // Number of transactions (already in header, but we serialize each tx).
    for (const auto& tx : transactions) {
        auto tx_bytes = tx.serialize();
        // Prefix each tx with its length for deserialization.
        uint32_t tx_len = static_cast<uint32_t>(tx_bytes.size());
        buf.push_back(static_cast<uint8_t>(tx_len >> 24));
        buf.push_back(static_cast<uint8_t>(tx_len >> 16));
        buf.push_back(static_cast<uint8_t>(tx_len >> 8));
        buf.push_back(static_cast<uint8_t>(tx_len));
        buf.insert(buf.end(), tx_bytes.begin(), tx_bytes.end());
    }

    return buf;
}

Block Block::deserialize(const std::vector<uint8_t>& data) {
    Block blk;

    // Deserialize header (fixed size portion).
    // Header: 4 + 8 + 32 + 32 + 32 + 8 + 4 + 8 = 128 bytes.
    std::vector<uint8_t> header_data(data.begin(), data.begin() + 128);
    blk.header = BlockHeader::deserialize(header_data);

    const uint8_t* p = data.data() + 128;

    for (uint32_t i = 0; i < blk.header.tx_count; ++i) {
        uint32_t tx_len = (static_cast<uint32_t>(p[0]) << 24)
                        | (static_cast<uint32_t>(p[1]) << 16)
                        | (static_cast<uint32_t>(p[2]) << 8)
                        | p[3];
        p += 4;
        std::vector<uint8_t> tx_data(p, p + tx_len);
        blk.transactions.push_back(Transaction::deserialize(tx_data));
        p += tx_len;
    }

    return blk;
}

std::string Block::block_id() const {
    return crypto::hash_to_hex(compute_hash());
}

size_t Block::size_bytes() const {
    return serialize().size();
}

} // namespace core
} // namespace kasturisundari
