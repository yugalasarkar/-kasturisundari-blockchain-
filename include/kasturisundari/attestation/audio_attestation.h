// Kasturisundari Chain — Audio Chanting Attestation (Shabda)
// Hashes raw PCM audio data of Vedic chanting to preserve the exact frequency
// and phonetic vibration (Shabda) on the blockchain immutably.

#pragma once

#include "kasturisundari/crypto/sha256.h"
#include <string>
#include <vector>

namespace kasturisundari {
namespace vedic {

struct AudioAttestation {
    std::string title;
    std::string chanter_name;
    uint32_t sample_rate;     // e.g., 44100 or 48000
    uint32_t bit_depth;       // e.g., 16 or 24
    uint32_t num_channels;    // e.g., 1 (Mono) or 2 (Stereo)
    
    // Merkle root of the audio chunks
    crypto::Hash256 merkle_root;
    
    // Overall file hash
    crypto::Hash256 full_audio_hash;

    /// Serialize for chain storage
    std::vector<uint8_t> serialize() const;
    static AudioAttestation deserialize(const std::vector<uint8_t>& data);
};

class ShabdaEngine {
public:
    /// Chunks a raw PCM audio buffer and creates a Merkle Tree.
    /// Returns the attestation structure ready to be placed in a transaction.
    static AudioAttestation create_attestation(
        const std::string& title,
        const std::string& chanter,
        uint32_t sample_rate,
        uint32_t bit_depth,
        uint32_t num_channels,
        const std::vector<uint8_t>& raw_pcm_data,
        size_t chunk_size = 4096 // 4KB chunks
    );

    /// Verifies that a given audio buffer matches the attestation exactly.
    static bool verify_audio(
        const AudioAttestation& attestation,
        const std::vector<uint8_t>& raw_pcm_data,
        size_t chunk_size = 4096
    );
};

} // namespace vedic
} // namespace kasturisundari
