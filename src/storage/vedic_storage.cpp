// Kasturisundari Chain — Vedic Storage Implementation

#include "kasturisundari/storage/vedic_storage.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace kasturisundari {
namespace storage {

VedicStorage::VedicStorage(const std::string& base_path) : base_path_(base_path) {}

bool VedicStorage::init() {
    try {
        if (!std::filesystem::exists(base_path_)) {
            std::filesystem::create_directories(base_path_);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[VedicStorage] Init Error: " << e.what() << "\n";
        return false;
    }
}

std::string VedicStorage::get_chunk_path(const crypto::Hash256& hash) const {
    std::string hex = crypto::hash_to_hex(hash);
    return base_path_ + "/" + hex + ".chunk";
}

bool VedicStorage::store_chunk(const std::vector<uint8_t>& data) {
    if (data.empty()) return false;

    crypto::Hash256 hash = crypto::sha256(data.data(), data.size());
    std::string path = get_chunk_path(hash);

    if (std::filesystem::exists(path)) {
        return true; // Already have it
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    return out.good();
}

std::vector<uint8_t> VedicStorage::get_chunk(const crypto::Hash256& chunk_hash) const {
    std::string path = get_chunk_path(chunk_hash);
    if (!std::filesystem::exists(path)) return {};

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return {};

    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (in.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return {};
}

bool VedicStorage::has_chunk(const crypto::Hash256& chunk_hash) const {
    return std::filesystem::exists(get_chunk_path(chunk_hash));
}

} // namespace storage
} // namespace kasturisundari
