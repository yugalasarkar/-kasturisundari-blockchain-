// Kasturisundari Chain — Sacred Text Attestation Implementation
// Provides immutable, tamper-proof preservation of sacred scriptures.

#include "kasturisundari/attestation/text_attestation.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>

namespace kasturisundari {
namespace attestation {

const char* category_to_string(TextCategory cat) {
    switch (cat) {
        case TextCategory::VEDA:          return "Veda";
        case TextCategory::UPANISHAD:     return "Upanishad";
        case TextCategory::PURANA:        return "Purana";
        case TextCategory::ITIHASA:       return "Itihasa";
        case TextCategory::SUTRA:         return "Sutra";
        case TextCategory::SMRITI:        return "Smriti";
        case TextCategory::STOTRA:        return "Stotra";
        case TextCategory::AGAMA:         return "Agama";
        case TextCategory::DHARMASHASTRA: return "Dharmashastra";
        case TextCategory::BHASHYA:       return "Bhashya";
        case TextCategory::OTHER:         return "Other";
        default:                          return "Unknown";
    }
}

crypto::Hash256 hash_verse(const std::string& content) {
    return crypto::sha256(content);
}

crypto::Hash256 compute_merkle_root(const std::vector<crypto::Hash256>& hashes) {
    if (hashes.empty()) {
        return crypto::Hash256{};
    }
    if (hashes.size() == 1) {
        return hashes[0];
    }

    std::vector<crypto::Hash256> current_level = hashes;

    while (current_level.size() > 1) {
        std::vector<crypto::Hash256> next_level;

        // If odd number of nodes, duplicate the last one.
        if (current_level.size() % 2 != 0) {
            current_level.push_back(current_level.back());
        }

        for (size_t i = 0; i < current_level.size(); i += 2) {
            // Concatenate the two sibling hashes and double-SHA256 them.
            std::vector<uint8_t> combined;
            combined.reserve(64);
            combined.insert(combined.end(),
                            current_level[i].begin(),
                            current_level[i].end());
            combined.insert(combined.end(),
                            current_level[i + 1].begin(),
                            current_level[i + 1].end());

            next_level.push_back(crypto::double_sha256(combined));
        }

        current_level = std::move(next_level);
    }

    return current_level[0];
}

ScriptureAttestation attest_scripture(
    const std::string& title,
    TextCategory category,
    const std::string& language,
    const std::vector<Verse>& verses) {

    ScriptureAttestation att;
    att.title    = title;
    att.category = category;
    att.language = language;

    // Current unix timestamp.
    att.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count());

    // Hash each verse individually.
    std::vector<crypto::Hash256> verse_hashes;
    std::string full_text;

    for (const auto& verse : verses) {
        VerseAttestation va;
        va.chapter      = verse.chapter;
        va.verse_num    = verse.verse_num;
        va.content_hash = hash_verse(verse.content);

        att.verses.push_back(va);
        verse_hashes.push_back(va.content_hash);

        full_text += verse.content;
    }

    // Compute the Merkle root of all verse hashes.
    att.merkle_root = compute_merkle_root(verse_hashes);

    // Also hash the entire concatenated text for a single fingerprint.
    att.full_text_hash = crypto::sha256(full_text);

    return att;
}

bool verify_verse(const std::string& content,
                  const crypto::Hash256& expected_hash) {
    crypto::Hash256 actual = hash_verse(content);
    return actual == expected_hash;
}

bool verify_scripture(const std::vector<Verse>& verses,
                      const ScriptureAttestation& attestation) {
    if (verses.size() != attestation.verses.size()) {
        return false;
    }

    // Verify each verse hash.
    std::vector<crypto::Hash256> verse_hashes;
    std::string full_text;

    for (size_t i = 0; i < verses.size(); ++i) {
        crypto::Hash256 h = hash_verse(verses[i].content);
        if (h != attestation.verses[i].content_hash) {
            return false;
        }
        verse_hashes.push_back(h);
        full_text += verses[i].content;
    }

    // Verify the Merkle root.
    crypto::Hash256 root = compute_merkle_root(verse_hashes);
    if (root != attestation.merkle_root) {
        return false;
    }

    // Verify the full text hash.
    crypto::Hash256 fth = crypto::sha256(full_text);
    if (fth != attestation.full_text_hash) {
        return false;
    }

    return true;
}

// ─── Serialization ──────────────────────────────────────────────────
// Binary format:
//   [2 bytes] title length
//   [N bytes] title (UTF-8)
//   [1 byte]  category
//   [2 bytes] language length
//   [N bytes] language (UTF-8)
//   [8 bytes] timestamp (big-endian)
//   [4 bytes] verse count (big-endian)
//   For each verse:
//     [4 bytes] chapter (big-endian)
//     [4 bytes] verse_num (big-endian)
//     [32 bytes] content_hash
//   [32 bytes] merkle_root
//   [32 bytes] full_text_hash

namespace {

void write_u16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val));
}

void write_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val >> 24));
    buf.push_back(static_cast<uint8_t>(val >> 16));
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val));
}

void write_u64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 7; i >= 0; --i) {
        buf.push_back(static_cast<uint8_t>(val >> (i * 8)));
    }
}

void write_bytes(std::vector<uint8_t>& buf,
                 const uint8_t* data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

void write_string(std::vector<uint8_t>& buf, const std::string& s) {
    write_u16(buf, static_cast<uint16_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

uint16_t read_u16(const uint8_t*& ptr) {
    uint16_t val = (static_cast<uint16_t>(ptr[0]) << 8) | ptr[1];
    ptr += 2;
    return val;
}

uint32_t read_u32(const uint8_t*& ptr) {
    uint32_t val = (static_cast<uint32_t>(ptr[0]) << 24)
                 | (static_cast<uint32_t>(ptr[1]) << 16)
                 | (static_cast<uint32_t>(ptr[2]) << 8)
                 | ptr[3];
    ptr += 4;
    return val;
}

uint64_t read_u64(const uint8_t*& ptr) {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val = (val << 8) | ptr[i];
    }
    ptr += 8;
    return val;
}

std::string read_string(const uint8_t*& ptr) {
    uint16_t len = read_u16(ptr);
    std::string s(reinterpret_cast<const char*>(ptr), len);
    ptr += len;
    return s;
}

} // anonymous namespace

std::vector<uint8_t> serialize_attestation(const ScriptureAttestation& att) {
    std::vector<uint8_t> buf;
    buf.reserve(256 + att.verses.size() * 40);

    write_string(buf, att.title);
    buf.push_back(static_cast<uint8_t>(att.category));
    write_string(buf, att.language);
    write_u64(buf, att.timestamp);
    write_u32(buf, static_cast<uint32_t>(att.verses.size()));

    for (const auto& v : att.verses) {
        write_u32(buf, v.chapter);
        write_u32(buf, v.verse_num);
        write_bytes(buf, v.content_hash.data(), 32);
    }

    write_bytes(buf, att.merkle_root.data(), 32);
    write_bytes(buf, att.full_text_hash.data(), 32);

    return buf;
}

ScriptureAttestation deserialize_attestation(const std::vector<uint8_t>& data) {
    ScriptureAttestation att;
    const uint8_t* ptr = data.data();

    att.title    = read_string(ptr);
    att.category = static_cast<TextCategory>(*ptr++);
    att.language = read_string(ptr);
    att.timestamp = read_u64(ptr);

    uint32_t verse_count = read_u32(ptr);
    att.verses.resize(verse_count);

    for (uint32_t i = 0; i < verse_count; ++i) {
        att.verses[i].chapter   = read_u32(ptr);
        att.verses[i].verse_num = read_u32(ptr);
        std::memcpy(att.verses[i].content_hash.data(), ptr, 32);
        ptr += 32;
    }

    std::memcpy(att.merkle_root.data(), ptr, 32);
    ptr += 32;
    std::memcpy(att.full_text_hash.data(), ptr, 32);

    return att;
}

} // namespace attestation
} // namespace kasturisundari
