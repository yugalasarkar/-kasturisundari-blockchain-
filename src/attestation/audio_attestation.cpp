// Kasturisundari Chain — Shabda Engine Implementation

#include "kasturisundari/attestation/audio_attestation.h"
#include "kasturisundari/core/block.h" // For Merkle Tree logic

#include <cstring>

namespace kasturisundari {
namespace vedic {

namespace {
void write_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val >> 24));
    buf.push_back(static_cast<uint8_t>(val >> 16));
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val));
}

void write_string(std::vector<uint8_t>& buf, const std::string& s) {
    write_u32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

uint32_t read_u32(const uint8_t*& p) {
    uint32_t v = (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | p[3];
    p += 4; return v;
}

std::string read_string(const uint8_t*& p) {
    uint32_t len = read_u32(p);
    std::string s(reinterpret_cast<const char*>(p), len);
    p += len; return s;
}
} // namespace

std::vector<uint8_t> AudioAttestation::serialize() const {
    std::vector<uint8_t> buf;
    write_string(buf, title);
    write_string(buf, chanter_name);
    write_u32(buf, sample_rate);
    write_u32(buf, bit_depth);
    write_u32(buf, num_channels);
    buf.insert(buf.end(), merkle_root.begin(), merkle_root.end());
    buf.insert(buf.end(), full_audio_hash.begin(), full_audio_hash.end());
    return buf;
}

AudioAttestation AudioAttestation::deserialize(const std::vector<uint8_t>& data) {
    AudioAttestation att;
    if (data.empty()) return att;
    const uint8_t* ptr = data.data();
    
    att.title = read_string(ptr);
    att.chanter_name = read_string(ptr);
    att.sample_rate = read_u32(ptr);
    att.bit_depth = read_u32(ptr);
    att.num_channels = read_u32(ptr);
    
    std::memcpy(att.merkle_root.data(), ptr, 32); ptr += 32;
    std::memcpy(att.full_audio_hash.data(), ptr, 32); ptr += 32;
    
    return att;
}

AudioAttestation ShabdaEngine::create_attestation(
    const std::string& title, const std::string& chanter,
    uint32_t sample_rate, uint32_t bit_depth, uint32_t num_channels,
    const std::vector<uint8_t>& raw_pcm_data, size_t chunk_size) 
{
    AudioAttestation att;
    att.title = title;
    att.chanter_name = chanter;
    att.sample_rate = sample_rate;
    att.bit_depth = bit_depth;
    att.num_channels = num_channels;

    std::vector<crypto::Hash256> chunk_hashes;
    size_t total_size = raw_pcm_data.size();
    
    for (size_t offset = 0; offset < total_size; offset += chunk_size) {
        size_t current_chunk_size = std::min(chunk_size, total_size - offset);
        
        // Hash the chunk
        crypto::Hash256 hash = crypto::sha256(
            raw_pcm_data.data() + offset,
            current_chunk_size
        );
        chunk_hashes.push_back(hash);
    }

    if (chunk_hashes.empty()) {
        att.merkle_root = crypto::Hash256{0};
    } else {
        // We reuse the blockchain's Merkle tree algorithm
        att.merkle_root = core::compute_tx_merkle_root(chunk_hashes);
    }

    att.full_audio_hash = crypto::sha256(
        raw_pcm_data.data(), 
        raw_pcm_data.size()
    );

    return att;
}

bool ShabdaEngine::verify_audio(
    const AudioAttestation& attestation,
    const std::vector<uint8_t>& raw_pcm_data,
    size_t chunk_size) 
{
    // Recreate the attestation
    AudioAttestation computed = create_attestation(
        attestation.title, attestation.chanter_name,
        attestation.sample_rate, attestation.bit_depth, attestation.num_channels,
        raw_pcm_data, chunk_size
    );

    return computed.full_audio_hash == attestation.full_audio_hash &&
           computed.merkle_root == attestation.merkle_root;
}

} // namespace vedic
} // namespace kasturisundari
