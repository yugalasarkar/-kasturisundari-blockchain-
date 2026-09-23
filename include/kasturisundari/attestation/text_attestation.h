// Kasturisundari Chain — Sacred Text Attestation System
// Immutable on-chain notarization of sacred scriptures.
// Computes cryptographic attestations for any text, allowing
// permanent, tamper-proof preservation on the blockchain.

#pragma once

#include "kasturisundari/crypto/sha256.h"

#include <cstdint>
#include <string>
#include <vector>

namespace kasturisundari {
namespace attestation {

/// Categories of sacred texts supported by the attestation system.
enum class TextCategory : uint8_t {
    VEDA          = 0x01,  // Rigveda, Yajurveda, Samaveda, Atharvaveda
    UPANISHAD     = 0x02,  // Principal and minor Upanishads
    PURANA        = 0x03,  // Vishnu Purana, Bhagavata Purana, etc.
    ITIHASA       = 0x04,  // Mahabharata, Ramayana
    SUTRA         = 0x05,  // Brahma Sutras, Yoga Sutras, etc.
    SMRITI        = 0x06,  // Manu Smriti, Yajnavalkya Smriti, etc.
    STOTRA        = 0x07,  // Vishnu Sahasranama, Lalita Sahasranama, etc.
    AGAMA         = 0x08,  // Pancharatra, Vaikhanasa, etc.
    DHARMASHASTRA = 0x09,  // Texts on Dharma and law
    BHASHYA       = 0x0A,  // Commentaries (Shankaracharya, Ramanujacharya, etc.)
    OTHER         = 0xFF   // Any other sacred or historical text
};

/// Convert a TextCategory enum to its human-readable name.
const char* category_to_string(TextCategory cat);

/// A single verse or passage within a larger scripture.
struct Verse {
    uint32_t    chapter;     // Chapter / Adhyaya / Mandala number
    uint32_t    verse_num;   // Verse / Shloka / Mantra number
    std::string content;     // The actual text content (UTF-8)
};

/// The attestation record for a single verse.
struct VerseAttestation {
    uint32_t         chapter;
    uint32_t         verse_num;
    crypto::Hash256  content_hash;   // SHA-256 of the verse content
};

/// A complete scripture attestation containing all verses and a root hash.
struct ScriptureAttestation {
    std::string                    title;          // e.g. "Rigveda Mandala 1"
    TextCategory                   category;       // Classification
    std::string                    language;        // e.g. "Sanskrit", "Tamil"
    uint64_t                       timestamp;       // Unix timestamp of attestation
    std::vector<VerseAttestation>  verses;          // Individual verse hashes
    crypto::Hash256                merkle_root;     // Merkle root of all verse hashes
    crypto::Hash256                full_text_hash;  // SHA-256 of the entire concatenated text
};

/// Compute the SHA-256 hash of a single verse's content.
crypto::Hash256 hash_verse(const std::string& content);

/// Compute the Merkle root from a list of verse hashes.
/// This creates a tamper-proof fingerprint of the entire scripture:
/// changing even one character in any verse will change the root.
crypto::Hash256 compute_merkle_root(const std::vector<crypto::Hash256>& hashes);

/// Create a full attestation for a scripture from its verses.
/// This produces the complete cryptographic proof that can be
/// stored on-chain to permanently protect the text.
ScriptureAttestation attest_scripture(
    const std::string& title,
    TextCategory category,
    const std::string& language,
    const std::vector<Verse>& verses);

/// Verify a single verse against its known attestation hash.
/// Returns true if the verse content matches the recorded hash.
bool verify_verse(const std::string& content,
                  const crypto::Hash256& expected_hash);

/// Verify an entire scripture against its attestation.
/// Rehashes every verse, recomputes the Merkle root, and checks
/// that it matches the recorded root. Returns true only if
/// every single character is identical to the attested version.
bool verify_scripture(const std::vector<Verse>& verses,
                      const ScriptureAttestation& attestation);

/// Serialize a ScriptureAttestation to binary format for on-chain storage.
std::vector<uint8_t> serialize_attestation(const ScriptureAttestation& att);

/// Deserialize a ScriptureAttestation from binary data.
ScriptureAttestation deserialize_attestation(const std::vector<uint8_t>& data);

} // namespace attestation
} // namespace kasturisundari
