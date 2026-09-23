// Kasturisundari Chain — JSON-RPC Server
// Zero-dependency HTTP server over POSIX sockets for JSON-RPC 2.0.
// Allows dApps, Wallets, and Explorers to interact with the node.

#pragma once

#include "kasturisundari/state/state_db.h"
#include "kasturisundari/mempool/mempool.h"
#include "kasturisundari/core/blockchain.h"
#include "kasturisundari/storage/vedic_storage.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <functional>

namespace kasturisundari {
namespace rpc {

/// A lightweight representation of a JSON-RPC request.
struct RpcRequest {
    std::string method;
    std::string params_raw; // The raw JSON string of parameters
    std::string id;
};

/// A lightweight representation of a JSON-RPC response.
struct RpcResponse {
    std::string id;
    std::string result_json; // If successful
    std::string error_json;  // If failed
    
    std::string to_json() const;
};

class RpcServer {
public:
    /// Requires references to core components to serve queries.
    RpcServer(uint16_t port, 
              state::StateDB& state, 
              mempool::Mempool& mempool, 
              core::Blockchain& chain,
              storage::VedicStorage& vstorage);
    
    ~RpcServer();

    bool start();
    void stop();

    /// Callback for when a new valid transaction is submitted via RPC
    void set_tx_broadcast_callback(std::function<void(const core::Transaction&)> cb);

private:
    uint16_t port_;
    std::function<void(const core::Transaction&)> on_new_tx_;
    std::atomic<bool> running_{false};
    int server_socket_{-1};
    std::thread listener_thread_;

    state::StateDB& state_;
    mempool::Mempool& mempool_;
    core::Blockchain& chain_;
    storage::VedicStorage& vstorage_;

    void listener_loop();
    
    /// Handle a single HTTP connection.
    void handle_client(int client_sock);

    /// Route the RPC method to the appropriate handler.
    RpcResponse handle_request(const RpcRequest& req);

    // --- RPC Method Handlers ---
    RpcResponse rpc_get_balance(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_get_block(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_send_transaction(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_get_info(const std::string& req_id);
    RpcResponse rpc_get_mempool(const std::string& req_id);
    RpcResponse rpc_get_recent_blocks(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_submit_proposal(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_cast_vote(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_get_proposals(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_store_chunk(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_get_chunk(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_get_history(const std::string& params_raw, const std::string& req_id);

    // --- Ethereum Standard Web3 JSON-RPC Handlers ---
    RpcResponse rpc_eth_chain_id(const std::string& req_id);
    RpcResponse rpc_eth_gas_price(const std::string& req_id);
    RpcResponse rpc_eth_accounts(const std::string& req_id);
    RpcResponse rpc_eth_block_number(const std::string& req_id);
    RpcResponse rpc_eth_get_block_by_number(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_block_by_hash(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_balance(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_transaction_count(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_code(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_storage_at(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_send_raw_transaction(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_call(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_estimate_gas(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_transaction_receipt(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_transaction_by_hash(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_logs(const std::string& params_raw, const std::string& req_id);
    RpcResponse rpc_eth_get_global_exit_root(const std::string& req_id);
    RpcResponse rpc_pol_get_staking_info(const std::string& req_id);
    RpcResponse rpc_net_version(const std::string& req_id);
    RpcResponse rpc_web3_client_version(const std::string& req_id);
};

/// Helper: Extremely basic JSON string extraction (zero-dependency).
/// Finds the string value associated with a key in a flat JSON object.
std::string extract_json_string(const std::string& json, const std::string& key);

} // namespace rpc
} // namespace kasturisundari
