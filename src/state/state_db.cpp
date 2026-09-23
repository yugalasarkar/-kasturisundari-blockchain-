// Kasturisundari Chain — State Database Implementation (Custom File-backed)

#include "kasturisundari/state/state_db.h"

#include <fstream>
#include <cstring>
#include <iostream>

namespace kasturisundari {
namespace state {

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

// ─── Account serialization ──────────────────────────────────────────

std::vector<uint8_t> Account::serialize() const {
    std::vector<uint8_t> buf;
    buf.reserve(16);
    write_u64(buf, balance);
    write_u64(buf, nonce);
    return buf;
}

Account Account::deserialize(const std::vector<uint8_t>& data) {
    Account acc{0, 0};
    if (data.size() < 16) return acc;
    const uint8_t* p = data.data();
    acc.balance = read_u64(p);
    acc.nonce   = read_u64(p);
    return acc;
}

// ─── StateDB ────────────────────────────────────────────────────────

StateDB::StateDB() = default;

StateDB::~StateDB() {
    close();
}

bool StateDB::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (is_open_) return false;

    db_path_ = path;
    is_open_ = true;
    
    // Load existing data if file exists, otherwise start fresh.
    load_from_disk();
    return true;
}

void StateDB::close() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!is_open_) return;
    save_to_disk();
    data_.clear();
    is_open_ = false;
}

bool StateDB::is_open() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return is_open_;
}

std::string StateDB::account_key(const std::string& address) const {
    return "acc:" + address;
}

std::string StateDB::meta_key(const std::string& key) const {
    return "meta:" + key;
}

bool StateDB::load_from_disk() {
    if (db_path_.empty() || db_path_ == ":memory:") return true;
    std::ifstream file(db_path_, std::ios::binary);
    if (!file) return false; // File probably doesn't exist yet, which is fine.

    data_.clear();
    
    // Read magic bytes
    char magic[4];
    if (!file.read(magic, 4) || std::strncmp(magic, "KAST", 4) != 0) {
        return false;
    }

    while (file.peek() != EOF) {
        uint32_t klen = 0;
        if (!file.read(reinterpret_cast<char*>(&klen), 4)) break;
        std::string key(klen, '\0');
        if (!file.read(&key[0], klen)) break;

        uint32_t vlen = 0;
        if (!file.read(reinterpret_cast<char*>(&vlen), 4)) break;
        std::string val(vlen, '\0');
        if (!file.read(&val[0], vlen)) break;

        data_[key] = val;
    }
    return true;
}

bool StateDB::save_to_disk() const {
    if (db_path_.empty() || db_path_ == ":memory:") return true;
    
    std::ofstream file(db_path_, std::ios::binary | std::ios::trunc);
    if (!file) return false;

    // Write magic bytes
    file.write("KAST", 4);

    for (const auto& [key, val] : data_) {
        uint32_t klen = key.size();
        file.write(reinterpret_cast<const char*>(&klen), 4);
        file.write(key.data(), klen);

        uint32_t vlen = val.size();
        file.write(reinterpret_cast<const char*>(&vlen), 4);
        file.write(val.data(), vlen);
    }
    return true;
}

Account StateDB::get_account(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!is_open_) return {0, 0};

    auto it = data_.find(account_key(address));
    if (it == data_.end()) return {0, 0};

    std::vector<uint8_t> data(it->second.begin(), it->second.end());
    return Account::deserialize(data);
}

bool StateDB::put_account(const std::string& address, const Account& account) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!is_open_) return false;

    auto data = account.serialize();
    data_[account_key(address)] = std::string(data.begin(), data.end());
    return true;
}

bool StateDB::has_account(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!is_open_) return false;
    return data_.count(account_key(address)) > 0;
}

uint64_t StateDB::get_balance(const std::string& address) const {
    return get_account(address).balance;
}

bool StateDB::add_transaction_history(const std::string& address, const std::string& txid) {
    std::string key = "hist_" + address;
    std::optional<std::string> current = get_meta(key);
    std::string new_hist = current ? *current + "," + txid : txid;
    return put_meta(key, new_hist);
}

std::string StateDB::get_transaction_history(const std::string& address) const {
    std::string key = "hist_" + address;
    std::optional<std::string> current = get_meta(key);
    return current ? *current : "";
}

bool StateDB::put_meta(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!is_open_) return false;
    data_[meta_key(key)] = value;
    return true;
}

std::optional<std::string> StateDB::get_meta(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!is_open_) return std::nullopt;

    auto it = data_.find(meta_key(key));
    if (it == data_.end()) return std::nullopt;
    return it->second;
}

bool StateDB::put_latest_block_hash(const crypto::Hash256& hash) {
    std::string hex = crypto::hash_to_hex(hash);
    return put_meta("latest_block_hash", hex);
}

std::optional<crypto::Hash256> StateDB::get_latest_block_hash() const {
    auto hex = get_meta("latest_block_hash");
    if (!hex) return std::nullopt;
    return crypto::hex_to_hash(*hex);
}

bool StateDB::put_chain_height(uint64_t height) {
    return put_meta("chain_height", std::to_string(height));
}

std::optional<uint64_t> StateDB::get_chain_height() const {
    auto val = get_meta("chain_height");
    if (!val) return std::nullopt;
    return std::stoull(*val);
}

namespace {
    static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> in_memory_contracts;
    static std::unordered_map<std::string, std::unordered_map<std::string, bool>> in_memory_akshara_registry;
}

bool StateDB::set_akshara_immutable(const std::string& contract_address, const std::string& key) {
    in_memory_akshara_registry[contract_address][key] = true;
    return true;
}

bool StateDB::is_akshara_immutable(const std::string& contract_address, const std::string& key) const {
    if (contract_address.rfind("0x416b", 0) == 0 || contract_address.rfind("0x416B", 0) == 0 ||
        contract_address.rfind("0xa1c5", 0) == 0 || contract_address.rfind("0xA1C5", 0) == 0) {
        return true;
    }
    if (key.rfind("0x416b", 0) == 0 || key.rfind("0x416B", 0) == 0 || key.rfind("akshara_", 0) == 0) {
        return true;
    }
    auto it_c = in_memory_akshara_registry.find(contract_address);
    if (it_c != in_memory_akshara_registry.end()) {
        if (it_c->second.count(key) && it_c->second.at(key)) return true;
        if (it_c->second.count("*") && it_c->second.at("*")) return true;
    }
    return false;
}

bool StateDB::set_contract_state(const std::string& contract_address, const std::string& key, const std::string& value) {
    std::string old_val = get_contract_state(contract_address, key);
    bool existed = (old_val != "0" && !old_val.empty() && old_val != "0x0000000000000000000000000000000000000000000000000000000000000000");

    if (is_akshara_immutable(contract_address, key) && existed && value != old_val) {
        std::cerr << "[Akshara State Engine] REJECTED: Attempted mutation of write-once immutable slot key='" 
                  << key << "' on contract=" << contract_address << "\n";
        return false;
    }

    in_memory_contracts[contract_address][key] = value;
    return true;
}

std::string StateDB::get_contract_state(const std::string& contract_address, const std::string& key) {
    if (in_memory_contracts.count(contract_address) && in_memory_contracts[contract_address].count(key)) {
        return in_memory_contracts[contract_address][key];
    }
    return "0";
}

} // namespace state
} // namespace kasturisundari

