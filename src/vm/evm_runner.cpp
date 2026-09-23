// Kasturisundari Chain — EVM Execution Engine Implementation

#include "kasturisundari/vm/evm_runner.h"
#include "kasturisundari/crypto/sha256.h"
#include "kasturisundari/economics/tokenomics.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace kasturisundari {
namespace vm {

// ─── Helpers ────────────────────────────────────────────────────────

std::string address_to_hex(const evmc::address& addr) {
    std::ostringstream oss;
    oss << "0x";
    for (size_t i = 0; i < sizeof(addr.bytes); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)addr.bytes[i];
    }
    return oss.str();
}

evmc::address hex_to_address(const std::string& hex_in) {
    std::string hex = hex_in;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) {
        hex = hex.substr(2);
    }
    evmc::address addr{};
    if (hex.length() < 40) return addr;
    
    for (size_t i = 0; i < 20; ++i) {
        std::string byteString = hex.substr(i * 2, 2);
        addr.bytes[i] = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
    }
    return addr;
}

std::string bytes32_to_hex(const evmc::bytes32& b32) {
    std::ostringstream oss;
    oss << "0x";
    for (size_t i = 0; i < 32; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)b32.bytes[i];
    }
    return oss.str();
}

evmc::bytes32 hex_to_bytes32(const std::string& hex_in) {
    std::string hex = hex_in;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) {
        hex = hex.substr(2);
    }
    evmc::bytes32 b32{};
    if (hex.length() < 64) return b32;
    
    for (size_t i = 0; i < 32; ++i) {
        std::string byteString = hex.substr(i * 2, 2);
        b32.bytes[i] = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
    }
    return b32;
}

// ─── KasturiEVMHost ─────────────────────────────────────────────────

KasturiEVMHost::KasturiEVMHost(state::StateDB& state, uint64_t height, uint64_t timestamp)
    : state_(state), block_height_(height), block_timestamp_(timestamp) {}

bool KasturiEVMHost::account_exists(const evmc::address& addr) const noexcept {
    std::string hex_addr = address_to_hex(addr);
    return state_.get_account(hex_addr).balance > 0 || !state_.get_contract_state(hex_addr, "bytecode").empty();
}

evmc::bytes32 KasturiEVMHost::get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept {
    std::string hex_addr = address_to_hex(addr);
    std::string hex_key = bytes32_to_hex(key);
    std::string val_hex = state_.get_contract_state(hex_addr, "storage_" + hex_key);
    if (val_hex == "0" || val_hex.empty()) {
        return evmc::bytes32{};
    }
    return hex_to_bytes32(val_hex);
}

evmc_storage_status KasturiEVMHost::set_storage(const evmc::address& addr,
                                               const evmc::bytes32& key,
                                               const evmc::bytes32& value) noexcept {
    std::string hex_addr = address_to_hex(addr);
    std::string hex_key = bytes32_to_hex(key);
    std::string hex_val = bytes32_to_hex(value);
    
    std::string old_val = state_.get_contract_state(hex_addr, "storage_" + hex_key);
    bool existed = (old_val != "0" && !old_val.empty() && old_val != "0x0000000000000000000000000000000000000000000000000000000000000000");
    
    if (state_.is_akshara_immutable(hex_addr, "storage_" + hex_key) || state_.is_akshara_immutable(hex_addr, hex_key)) {
        if (existed && hex_val != old_val) {
            std::cerr << "[Akshara State Engine] Blocked storage mutation on immutable slot " << hex_key << "\n";
            akshara_revert_flag = true;
            akshara_revert_reason = "EXECUTION_REVERTED_AKSHARA_IMMUTABILITY";
            return EVMC_STORAGE_ASSIGNED;
        }
    }

    state_.set_contract_state(hex_addr, "storage_" + hex_key, hex_val);
    
    if (!existed && hex_val != "0x0000000000000000000000000000000000000000000000000000000000000000") {
        return EVMC_STORAGE_ADDED;
    }
    return EVMC_STORAGE_MODIFIED;
}

evmc::uint256be KasturiEVMHost::get_balance(const evmc::address& addr) const noexcept {
    std::string hex_addr = address_to_hex(addr);
    uint64_t bal = state_.get_balance(hex_addr);
    
    evmc::uint256be res{};
    for (int i = 0; i < 8; ++i) {
        res.bytes[31 - i] = (uint8_t)(bal >> (i * 8));
    }
    return res;
}

size_t KasturiEVMHost::get_code_size(const evmc::address& addr) const noexcept {
    std::string hex_addr = address_to_hex(addr);
    std::string code_hex = state_.get_contract_state(hex_addr, "bytecode");
    if (code_hex == "0" || code_hex.empty()) return 0;
    return code_hex.length() / 2;
}

evmc::bytes32 KasturiEVMHost::get_code_hash(const evmc::address& addr) const noexcept {
    std::string hex_addr = address_to_hex(addr);
    std::string code_hex = state_.get_contract_state(hex_addr, "bytecode");
    if (code_hex == "0" || code_hex.empty()) return evmc::bytes32{};
    
    crypto::Hash256 h = crypto::sha256((const uint8_t*)code_hex.data(), code_hex.length());
    evmc::bytes32 res{};
    std::memcpy(res.bytes, h.data(), 32);
    return res;
}

size_t KasturiEVMHost::copy_code(const evmc::address& addr,
                                  size_t code_offset,
                                  uint8_t* buffer_data,
                                  size_t buffer_size) const noexcept {
    std::string hex_addr = address_to_hex(addr);
    std::string code_hex = state_.get_contract_state(hex_addr, "bytecode");
    if (code_hex == "0" || code_hex.empty()) return 0;
    
    size_t total_bytes = code_hex.length() / 2;
    if (code_offset >= total_bytes) return 0;
    
    size_t copy_len = std::min(buffer_size, total_bytes - code_offset);
    for (size_t i = 0; i < copy_len; ++i) {
        std::string byteStr = code_hex.substr((code_offset + i) * 2, 2);
        buffer_data[i] = (uint8_t)strtol(byteStr.c_str(), nullptr, 16);
    }
    return copy_len;
}

bool KasturiEVMHost::selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept {
    std::string hex_addr = address_to_hex(addr);
    std::string hex_ben = address_to_hex(beneficiary);
    uint64_t bal = state_.get_balance(hex_addr);
    if (bal > 0) {
        uint64_t ben_bal = state_.get_balance(hex_ben);
        state::Account acc_ben = state_.get_account(hex_ben);
        acc_ben.balance += bal;
        state_.put_account(hex_ben, acc_ben);
        
        state::Account acc_src = state_.get_account(hex_addr);
        acc_src.balance = 0;
        state_.put_account(hex_addr, acc_src);
    }
    return true;
}

evmc::Result KasturiEVMHost::call(const evmc_message& msg) noexcept {
    evmc_result res{};
    res.status_code = EVMC_SUCCESS;
    res.gas_left = msg.gas;
    return evmc::Result(res);
}

evmc_tx_context KasturiEVMHost::get_tx_context() const noexcept {
    evmc_tx_context ctx{};
    ctx.block_number = (int64_t)block_height_;
    ctx.block_timestamp = (int64_t)block_timestamp_;
    ctx.block_gas_limit = 30'000'000;
    ctx.chain_id = evmc::uint256be{};
    uint64_t cid = economics::EVM_CHAIN_ID;
    for (size_t i = 0; i < 8; ++i) {
        ctx.chain_id.bytes[31 - i] = static_cast<uint8_t>(cid >> (i * 8));
    }
    return ctx;
}

evmc::bytes32 KasturiEVMHost::get_block_hash(int64_t number) const noexcept {
    return evmc::bytes32{};
}

void KasturiEVMHost::emit_log(const evmc::address& addr,
                              const uint8_t* data,
                              size_t data_size,
                              const evmc::bytes32 topics[],
                              size_t topics_count) noexcept {
    EVMLog log;
    log.address = addr;
    if (data && data_size > 0) {
        log.data.assign(data, data + data_size);
    }
    for (size_t i = 0; i < topics_count; ++i) {
        log.topics.push_back(topics[i]);
    }
    logs_.push_back(log);
}

// ─── EVM Execution ──────────────────────────────────────────────────

EVMResult execute_evm_tx(state::StateDB& state,
                         const evmc::address& sender,
                         const evmc::address& recipient,
                         const std::vector<uint8_t>& code_or_data,
                         uint64_t value_wei,
                         int64_t gas_limit,
                         uint64_t block_height,
                         uint64_t block_timestamp,
                         bool is_create) {
    EVMResult out{};
    
    KasturiEVMHost host(state, block_height, block_timestamp);
    evmc::VM vm = evmc::VM{evmc_create_evmone()};
    
    evmc_message msg{};
    msg.kind = is_create ? EVMC_CREATE : EVMC_CALL;
    msg.flags = 0;
    msg.depth = 0;
    msg.gas = gas_limit;
    msg.recipient = recipient;
    msg.sender = sender;
    msg.input_data = code_or_data.data();
    msg.input_size = code_or_data.size();
    
    std::vector<uint8_t> exec_code = code_or_data;
    if (!is_create) {
        std::string hex_rec = address_to_hex(recipient);
        std::string code_hex = state.get_contract_state(hex_rec, "bytecode");
        if (code_hex != "0" && !code_hex.empty()) {
            exec_code.clear();
            for (size_t i = 0; i < code_hex.length(); i += 2) {
                std::string byteStr = code_hex.substr(i, 2);
                exec_code.push_back((uint8_t)strtol(byteStr.c_str(), nullptr, 16));
            }
        }
    }

    evmc::Result res = vm.execute(host, EVMC_CANCUN, msg, exec_code.data(), exec_code.size());
    
    out.status_code = host.akshara_revert_flag ? EVMC_REVERT : res.status_code;
    out.gas_left = host.akshara_revert_flag ? 0 : res.gas_left;
    out.gas_refund = res.gas_refund;
    out.logs = host.get_logs();
    
    if (host.akshara_revert_flag) {
        out.output.assign(host.akshara_revert_reason.begin(), host.akshara_revert_reason.end());
    } else if (res.output_data && res.output_size > 0) {
        out.output.assign(res.output_data, res.output_data + res.output_size);
    }
    
    if (is_create && res.status_code == EVMC_SUCCESS) {
        std::string sender_hex = address_to_hex(sender);
        uint64_t nonce = state.get_account(sender_hex).nonce;
        std::string raw = sender_hex + ":" + std::to_string(nonce);
        crypto::Hash256 h = crypto::sha256((const uint8_t*)raw.data(), raw.length());
        
        evmc::address created_addr{};
        std::memcpy(created_addr.bytes, h.data() + 12, 20); // Last 20 bytes
        out.create_address = created_addr;
        
        std::string created_hex = address_to_hex(created_addr);
        std::string deployed_bytecode;
        for (uint8_t b : out.output) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", b);
            deployed_bytecode += buf;
        }
        state.set_contract_state(created_hex, "bytecode", deployed_bytecode);
        std::cout << "[EVM Engine] Deployed Contract at: " << created_hex 
                  << " | Bytecode size: " << out.output.size() << " bytes\n";
    }
    
    return out;
}

} // namespace vm
} // namespace kasturisundari
