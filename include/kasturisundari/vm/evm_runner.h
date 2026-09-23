// Kasturisundari Chain — EVM Execution Engine (evmone Integration)

#pragma once

#include <evmc/evmc.hpp>
#include <evmone/evmone.h>
#include "kasturisundari/state/state_db.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace kasturisundari {
namespace vm {

/// Represents an EVM Event Log (emitted via LOG0 - LOG4 opcodes)
struct EVMLog {
    evmc::address address;
    std::vector<uint8_t> data;
    std::vector<evmc::bytes32> topics;
};

/// Host Implementation for evmc callbacks
class KasturiEVMHost : public evmc::Host {
public:
    KasturiEVMHost(state::StateDB& state, uint64_t height, uint64_t timestamp);

    bool account_exists(const evmc::address& addr) const noexcept override;

    evmc::bytes32 get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept override;

    evmc_storage_status set_storage(const evmc::address& addr,
                                   const evmc::bytes32& key,
                                   const evmc::bytes32& value) noexcept override;

    evmc::uint256be get_balance(const evmc::address& addr) const noexcept override;

    size_t get_code_size(const evmc::address& addr) const noexcept override;

    evmc::bytes32 get_code_hash(const evmc::address& addr) const noexcept override;

    size_t copy_code(const evmc::address& addr,
                      size_t code_offset,
                      uint8_t* buffer_data,
                      size_t buffer_size) const noexcept override;

    bool selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept override;

    evmc::Result call(const evmc_message& msg) noexcept override;

    evmc_tx_context get_tx_context() const noexcept override;

    evmc::bytes32 get_block_hash(int64_t number) const noexcept override;

    void emit_log(const evmc::address& addr,
                  const uint8_t* data,
                  size_t data_size,
                  const evmc::bytes32 topics[],
                  size_t topics_count) noexcept override;

    evmc::bytes32 get_transient_storage(const evmc::address& addr,
                                         const evmc::bytes32& key) const noexcept override {
        auto it = transient_storage_.find(key);
        if (it != transient_storage_.end()) return it->second;
        return evmc::bytes32{};
    }

    void set_transient_storage(const evmc::address& addr,
                               const evmc::bytes32& key,
                               const evmc::bytes32& value) noexcept override {
        transient_storage_[key] = value;
    }

    evmc_access_status access_account(const evmc::address& addr) noexcept override {
        return EVMC_ACCESS_WARM;
    }

    evmc_access_status access_storage(const evmc::address& addr, const evmc::bytes32& key) noexcept override {
        return EVMC_ACCESS_WARM;
    }

    const std::vector<EVMLog>& get_logs() const { return logs_; }

    mutable bool akshara_revert_flag = false;
    mutable std::string akshara_revert_reason;

private:
    state::StateDB& state_;
    uint64_t block_height_;
    uint64_t block_timestamp_;
    std::vector<EVMLog> logs_;
    mutable std::unordered_map<evmc::bytes32, evmc::bytes32> transient_storage_;
};

/// EVM Execution Result
struct EVMResult {
    evmc_status_code status_code;
    int64_t gas_left;
    int64_t gas_refund;
    std::vector<uint8_t> output;
    evmc::address create_address;
    std::vector<EVMLog> logs;
};

/// Helper functions for hex address / bytes conversion
std::string address_to_hex(const evmc::address& addr);
evmc::address hex_to_address(const std::string& hex);
std::string bytes32_to_hex(const evmc::bytes32& b32);
evmc::bytes32 hex_to_bytes32(const std::string& hex);

/// Execute an EVM Transaction via evmone
EVMResult execute_evm_tx(state::StateDB& state,
                         const evmc::address& sender,
                         const evmc::address& recipient,
                         const std::vector<uint8_t>& code_or_data,
                         uint64_t value_wei,
                         int64_t gas_limit,
                         uint64_t block_height,
                         uint64_t block_timestamp,
                         bool is_create = false);

} // namespace vm
} // namespace kasturisundari
