// Kasturisundari Chain — State Database (Custom File-backed)
// Persistent storage layer using a custom binary file format.
// Stores account balances, nonces, and chain metadata.
// OPSEC: Zero external dependencies for storage.

#pragma once

#include "kasturisundari/crypto/sha256.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace kasturisundari {
namespace state {

/// An account on the Kasturisundari Chain.
struct Account {
    uint64_t balance;    // Balance in smallest unit (1e-8 NSK)
    uint64_t nonce;      // Transaction counter (prevents replay attacks)

    /// Serialize to binary.
    std::vector<uint8_t> serialize() const;

    /// Deserialize from binary.
    static Account deserialize(const std::vector<uint8_t>& data);
};

/// Persistent state database for the chain.
/// Uses an in-memory map backed by a binary file.
class StateDB {
public:
    StateDB();
    ~StateDB();

    /// Open (or create) a database at the given path.
    bool open(const std::string& path);

    /// Close the database and flush to disk.
    void close();

    /// Check if the database is open.
    bool is_open() const;

    // ─── Account Operations ─────────────────────────────────────

    /// Get an account by address. Returns default (0 balance, 0 nonce) if not found.
    Account get_account(const std::string& address) const;

    /// Set (overwrite) an account.
    bool put_account(const std::string& address, const Account& account);

    /// Check if an account exists.
    bool has_account(const std::string& address) const;

    // Smart Contract Agni VM Storage
    bool set_contract_state(const std::string& contract_address, const std::string& key, const std::string& value);
    std::string get_contract_state(const std::string& contract_address, const std::string& key);

    // ─── Akshara State Engine (Write-Once Pure Storage) ──────────
    bool set_akshara_immutable(const std::string& contract_address, const std::string& key);
    bool is_akshara_immutable(const std::string& contract_address, const std::string& key) const;

    /// Get the balance of an address.
    uint64_t get_balance(const std::string& address) const;

    // ─── Transaction History ────────────────────────────────────
    
    /// Add a transaction ID to an address's history.
    bool add_transaction_history(const std::string& address, const std::string& txid);
    
    /// Retrieve the CSV list of transaction IDs for an address.
    std::string get_transaction_history(const std::string& address) const;

    // ─── Chain Metadata ─────────────────────────────────────────

    /// Store a key-value pair in the metadata namespace.
    bool put_meta(const std::string& key, const std::string& value);

    /// Retrieve a metadata value.
    std::optional<std::string> get_meta(const std::string& key) const;

    /// Store the latest block hash.
    bool put_latest_block_hash(const crypto::Hash256& hash);

    /// Retrieve the latest block hash.
    std::optional<crypto::Hash256> get_latest_block_hash() const;

    /// Store the current chain height.
    bool put_chain_height(uint64_t height);

    /// Retrieve the current chain height.
    std::optional<uint64_t> get_chain_height() const;

private:
    bool is_open_ = false;
    std::string db_path_;
    mutable std::mutex mtx_;

    std::unordered_map<std::string, std::string> data_;

    bool load_from_disk();
    bool save_to_disk() const;

    std::string account_key(const std::string& address) const;
    std::string meta_key(const std::string& key) const;
};

} // namespace state
} // namespace kasturisundari
