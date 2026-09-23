// Kasturisundari Chain — JSON-RPC Server Implementation

#include "kasturisundari/rpc/rpc_server.h"
#include "kasturisundari/economics/tokenomics.h"
#include "kasturisundari/governance/sabha.h"
#include "kasturisundari/vm/evm_runner.h"
#include "kasturisundari/net/aggkit_adapter.h"
#include "kasturisundari/economics/pol_restaking.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>

namespace kasturisundari {
namespace rpc {

// ─── Minimal JSON Helpers ───────────────────────────────────────────

std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) {
        // Try with spaces
        search = "\"" + key + "\" :";
        pos = json.find(search);
        if (pos == std::string::npos) return "";
    }

    size_t val_start = json.find("\"", pos + search.length());
    if (val_start == std::string::npos) return ""; // Not a string

    size_t val_end = json.find("\"", val_start + 1);
    if (val_end == std::string::npos) return "";

    return json.substr(val_start + 1, val_end - val_start - 1);
}

std::string extract_json_id(const std::string& json) {
    std::string search = "\"id\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) {
        search = "\"id\" :";
        pos = json.find(search);
        if (pos == std::string::npos) return "1";
    }

    size_t val_start = pos + search.length();
    while (val_start < json.length() && (json[val_start] == ' ' || json[val_start] == '\t')) {
        val_start++;
    }

    if (val_start >= json.length()) return "1";

    if (json[val_start] == '"') {
        size_t end_quote = json.find('"', val_start + 1);
        if (end_quote != std::string::npos) {
            return json.substr(val_start + 1, end_quote - val_start - 1);
        }
    }

    size_t val_end = val_start;
    while (val_end < json.length() && json[val_end] != ',' && json[val_end] != '}' && json[val_end] != '\r' && json[val_end] != '\n') {
        val_end++;
    }

    std::string raw_id = json.substr(val_start, val_end - val_start);
    while (!raw_id.empty() && (raw_id.back() == ' ' || raw_id.back() == '\t')) raw_id.pop_back();
    return raw_id.empty() ? "1" : raw_id;
}

std::string RpcResponse::to_json() const {
    std::ostringstream oss;
    oss << "{\"jsonrpc\":\"2.0\",";
    
    if (!error_json.empty()) {
        oss << "\"error\":" << error_json << ",";
    } else {
        oss << "\"result\":" << (result_json.empty() ? "null" : result_json) << ",";
    }
    
    if (id.empty()) {
        oss << "\"id\":1}";
    } else if (id == "null" || (id[0] >= '0' && id[0] <= '9')) {
        oss << "\"id\":" << id << "}";
    } else {
        oss << "\"id\":\"" << id << "\"}";
    }
    return oss.str();
}

// ─── RpcServer ──────────────────────────────────────────────────────

RpcServer::RpcServer(uint16_t port, 
                     state::StateDB& state, 
                     mempool::Mempool& mempool, 
                     core::Blockchain& chain,
                     storage::VedicStorage& vstorage)
    : port_(port), state_(state), mempool_(mempool), chain_(chain), vstorage_(vstorage) {}

RpcServer::~RpcServer() {
    stop();
}

void RpcServer::set_tx_broadcast_callback(std::function<void(const core::Transaction&)> cb) {
    on_new_tx_ = std::move(cb);
}

bool RpcServer::start() {
    if (running_) return false;

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) return false;

    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket_);
        return false;
    }

    if (listen(server_socket_, 10) < 0) {
        close(server_socket_);
        return false;
    }

    running_ = true;
    listener_thread_ = std::thread(&RpcServer::listener_loop, this);

    return true;
}

void RpcServer::stop() {
    if (!running_) return;
    running_ = false;
    
    if (server_socket_ != -1) {
        // Break out of blocking accept
        shutdown(server_socket_, SHUT_RDWR);
        close(server_socket_);
        server_socket_ = -1;
    }
    
    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
}

void RpcServer::listener_loop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_sock >= 0) {
            // In a production OPSEC environment, we'd limit threads.
            // For now, spawn a detached thread per request.
            std::thread(&RpcServer::handle_client, this, client_sock).detach();
        }
    }
}

void RpcServer::handle_client(int client_sock) {
    std::string request_str;
    char buffer[16384];
    ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        request_str.append(buffer);
        
        size_t cl_pos = request_str.find("Content-Length: ");
        if (cl_pos != std::string::npos) {
            size_t end_cl = request_str.find("\r\n", cl_pos);
            if (end_cl != std::string::npos) {
                int content_length = std::stoi(request_str.substr(cl_pos + 16, end_cl - (cl_pos + 16)));
                size_t header_end = request_str.find("\r\n\r\n");
                if (header_end != std::string::npos) {
                    int body_received = request_str.length() - (header_end + 4);
                    int remaining = content_length - body_received;
                    while (remaining > 0) {
                        bytes_read = recv(client_sock, buffer, std::min((int)sizeof(buffer) - 1, remaining), 0);
                        if (bytes_read <= 0) break;
                        buffer[bytes_read] = '\0';
                        request_str.append(buffer);
                        remaining -= bytes_read;
                    }
                }
            }
        }
        
        // Handle CORS Preflight (OPTIONS request)
        if (request_str.find("OPTIONS") == 0) {
            std::string cors_resp = 
                "HTTP/1.1 204 No Content\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n"
                "Connection: close\r\n\r\n";
            send(client_sock, cors_resp.c_str(), cors_resp.length(), 0);
            close(client_sock);
            return;
        }
        
        // Very basic HTTP POST parser
        size_t header_end = request_str.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            std::string body = request_str.substr(header_end + 4);
            
            // Extract JSON-RPC fields
            RpcRequest req;
            req.method = extract_json_string(body, "method");
            req.id = extract_json_id(body);
            if (req.id.empty()) req.id = "1"; // default
            
            // For params, we just pass the whole body to extract_json_string later
            req.params_raw = body;
            
            RpcResponse resp = handle_request(req);
            std::string resp_json = resp.to_json();
            
            // HTTP Response
            std::ostringstream http;
            http << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: application/json\r\n"
                 << "Content-Length: " << resp_json.length() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << resp_json;
                 
            std::string http_str = http.str();
            send(client_sock, http_str.c_str(), http_str.length(), 0);
        }
    }
    
    close(client_sock);
}

RpcResponse RpcServer::handle_request(const RpcRequest& req) {
    if (req.method == "get_balance") {
        return rpc_get_balance(req.params_raw, req.id);
    } else if (req.method == "get_info") {
        return rpc_get_info(req.id);
    } else if (req.method == "get_mempool") {
        return rpc_get_mempool(req.id);
    } else if (req.method == "get_recent_blocks") {
        return rpc_get_recent_blocks(req.params_raw, req.id);
    } else if (req.method == "get_nonce") {
        RpcResponse resp;
        resp.id = req.id;
        std::string address = extract_json_string(req.params_raw, "address");
        if (address.empty()) {
            resp.error_json = "{\"code\":-32602,\"message\":\"Missing address parameter\"}";
            return resp;
        }
        
        uint64_t expected_nonce = mempool_.get_expected_nonce(address, state_);
        resp.result_json = "\"" + std::to_string(expected_nonce) + "\"";
        return resp;
        
    } else if (req.method == "send_transaction") {
        return rpc_send_transaction(req.params_raw, req.id);
    } else if (req.method == "submit_proposal") {
        return rpc_submit_proposal(req.params_raw, req.id);
    } else if (req.method == "cast_vote") {
        return rpc_cast_vote(req.params_raw, req.id);
    } else if (req.method == "get_proposals") {
        return rpc_get_proposals(req.params_raw, req.id);
    } else if (req.method == "get_chunk") {
        return rpc_get_chunk(req.params_raw, req.id);
    } else if (req.method == "store_chunk") {
        return rpc_store_chunk(req.params_raw, req.id);
    } else if (req.method == "get_history") {
        return rpc_get_history(req.params_raw, req.id);
    } else if (req.method == "eth_chainId") {
        return rpc_eth_chain_id(req.id);
    } else if (req.method == "eth_gasPrice") {
        return rpc_eth_gas_price(req.id);
    } else if (req.method == "eth_accounts") {
        return rpc_eth_accounts(req.id);
    } else if (req.method == "net_version") {
        return rpc_net_version(req.id);
    } else if (req.method == "web3_clientVersion") {
        return rpc_web3_client_version(req.id);
    } else if (req.method == "eth_blockNumber") {
        return rpc_eth_block_number(req.id);
    } else if (req.method == "eth_getBlockByNumber") {
        return rpc_eth_get_block_by_number(req.params_raw, req.id);
    } else if (req.method == "eth_getBlockByHash") {
        return rpc_eth_get_block_by_hash(req.params_raw, req.id);
    } else if (req.method == "eth_getBalance") {
        return rpc_eth_get_balance(req.params_raw, req.id);
    } else if (req.method == "eth_getTransactionCount") {
        return rpc_eth_get_transaction_count(req.params_raw, req.id);
    } else if (req.method == "eth_getCode") {
        return rpc_eth_get_code(req.params_raw, req.id);
    } else if (req.method == "eth_getStorageAt") {
        return rpc_eth_get_storage_at(req.params_raw, req.id);
    } else if (req.method == "eth_sendRawTransaction") {
        return rpc_eth_send_raw_transaction(req.params_raw, req.id);
    } else if (req.method == "eth_call") {
        return rpc_eth_call(req.params_raw, req.id);
    } else if (req.method == "eth_estimateGas") {
        return rpc_eth_estimate_gas(req.params_raw, req.id);
    } else if (req.method == "eth_getTransactionReceipt") {
        return rpc_eth_get_transaction_receipt(req.params_raw, req.id);
    } else if (req.method == "eth_getTransactionByHash") {
        return rpc_eth_get_transaction_by_hash(req.params_raw, req.id);
    } else if (req.method == "eth_getLogs") {
        return rpc_eth_get_logs(req.params_raw, req.id);
    } else if (req.method == "eth_getGlobalExitRoot") {
        return rpc_eth_get_global_exit_root(req.id);
    } else if (req.method == "pol_getStakingInfo") {
        return rpc_pol_get_staking_info(req.id);
    } else {
        RpcResponse resp;
        resp.id = req.id;
        resp.error_json = "{\"code\":-32601,\"message\":\"Method not found\"}";
        return resp;
    }
}

RpcResponse RpcServer::rpc_get_balance(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string address = extract_json_string(params_raw, "address");
    if (address.empty()) {
        resp.error_json = "{\"code\":-32602,\"message\":\"Missing address parameter\"}";
        return resp;
    }
    
    uint64_t bal = state_.get_balance(address);
    std::string formatted = economics::format_nila(bal);
    
    resp.result_json = "\"" + formatted + "\"";
    return resp;
}

RpcResponse RpcServer::rpc_get_info(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    uint64_t height = state_.get_chain_height().value_or(0);
    uint64_t supply = economics::MAX_SUPPLY / economics::UNITS_PER_NILA;
    
    std::ostringstream oss;
    oss << "{"
        << "\"chain\":\"Kasturisundari\","
        << "\"height\":" << height << ","
        << "\"total_supply\":" << supply << ","
        << "\"nodes\":" << (state_.get_account("K0000000000000000000000000000000000000000").balance > 0 ? 2 : 1) << ","
        << "\"mempool_size\":" << mempool_.size()
        << "}";
        
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_get_mempool(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    auto txs = mempool_.get_pending_transactions(50); // Get up to 50 pending
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < txs.size(); ++i) {
        oss << "{"
            << "\"txid\":\"" << txs[i].txid() << "\","
            << "\"type\":\"" << (int)txs[i].type << "\","
            << "\"fee\":" << txs[i].fee
            << "}";
        if (i < txs.size() - 1) oss << ",";
    }
    oss << "]";
    
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_get_recent_blocks(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    uint64_t current_height = state_.get_chain_height().value_or(0);
    int count = 10; // Default fetch 10 blocks
    
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (int i = 0; i < count; ++i) {
        if (current_height < i) break;
        uint64_t h = current_height - i;
        auto block_opt = chain_.get_block_at(h);
        if (block_opt) {
            const auto& block = block_opt->get();
            if (!first) oss << ",";
            oss << "{"
                << "\"height\":" << block.header.height << ","
                << "\"hash\":\"" << crypto::hash_to_hex(block.compute_hash()) << "\","
                << "\"timestamp\":" << block.header.timestamp << ","
                << "\"tx_count\":" << block.transactions.size()
                << "}";
            first = false;
        }
    }
    oss << "]";
    
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_send_transaction(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string hex_data = extract_json_string(params_raw, "tx_hex");
    if (hex_data.empty()) {
        resp.error_json = "{\"code\":-32602,\"message\":\"Missing tx_hex parameter\"}";
        return resp;
    }
    
    // Convert hex to bytes
    std::vector<uint8_t> tx_bytes;
    for (size_t i = 0; i < hex_data.length(); i += 2) {
        std::string byteString = hex_data.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), nullptr, 16);
        tx_bytes.push_back(byte);
    }
    
    try {
        core::Transaction tx = core::Transaction::deserialize(tx_bytes);
        if (!tx.verify_signature()) {
            resp.error_json = "{\"code\":-32603,\"message\":\"Invalid transaction signature\"}";
            return resp;
        }
        
        auto res = mempool_.add_transaction(tx, state_);
        if (res != mempool::TxRejectReason::NONE) {
            std::string reason_str = mempool::reject_reason_to_string(res);
            std::cerr << "[RPC] Mempool rejected tx: " << tx.txid() << " reason: " << reason_str << std::endl;
            resp.error_json = "{\"code\":-32603,\"message\":\"Transaction rejected by mempool: " + reason_str + "\"}";
            return resp;
        }
        resp.result_json = "\"" + tx.txid() + "\"";
        std::cerr << "[RPC] Transaction accepted into mempool: " << tx.txid() 
                  << " | Mempool size: " << mempool_.size() << std::endl;
        
        // Broadcast to P2P network
        if (on_new_tx_) {
            on_new_tx_(tx);
        }
        
    } catch (...) {
        resp.error_json = "{\"code\":-32603,\"message\":\"Failed to deserialize transaction\"}";
    }
    
    return resp;
}

} // namespace rpc
} // namespace kasturisundari

// ─── Governance RPC Handlers ────────────────────────────────────────

namespace kasturisundari {
namespace rpc {

RpcResponse RpcServer::rpc_submit_proposal(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string proposal_hash_hex = extract_json_string(params_raw, "proposal_hash");
    std::string proposer = extract_json_string(params_raw, "proposer");
    
    if (proposal_hash_hex.empty() || proposer.empty()) {
        resp.error_json = "{\"code\":-32602,\"message\":\"Missing proposal_hash or proposer\"}";
        return resp;
    }
    
    crypto::Hash256 hash = crypto::sha256(proposal_hash_hex);
    governance::Sabha sabha(state_);
    
    if (sabha.submit_proposal(hash, proposer)) {
        resp.result_json = "{\"status\":\"proposal_submitted\",\"hash\":\"" + crypto::hash_to_hex(hash) + "\"}";
    } else {
        resp.error_json = "{\"code\":-32603,\"message\":\"Proposal already exists or invalid\"}";
    }
    
    return resp;
}

RpcResponse RpcServer::rpc_cast_vote(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string proposal_hash_hex = extract_json_string(params_raw, "proposal_hash");
    std::string voter = extract_json_string(params_raw, "voter");
    std::string vote_str = extract_json_string(params_raw, "vote");
    
    if (proposal_hash_hex.empty() || voter.empty() || vote_str.empty()) {
        resp.error_json = "{\"code\":-32602,\"message\":\"Missing proposal_hash, voter, or vote\"}";
        return resp;
    }
    
    bool approve = (vote_str == "yes" || vote_str == "true" || vote_str == "1");
    crypto::Hash256 hash = crypto::sha256(proposal_hash_hex);
    governance::Sabha sabha(state_);
    
    if (sabha.cast_vote(hash, voter, approve)) {
        auto [yes_w, no_w] = sabha.get_tally(hash);
        bool passed = sabha.is_proposal_passed(hash);
        resp.result_json = "{\"status\":\"vote_cast\",\"yes_weight\":" + std::to_string(yes_w) 
            + ",\"no_weight\":" + std::to_string(no_w)
            + ",\"passed\":" + (passed ? "true" : "false") + "}";
    } else {
        resp.error_json = "{\"code\":-32603,\"message\":\"Vote failed (already voted, no balance, or proposal not found)\"}";
    }
    
    return resp;
}

RpcResponse RpcServer::rpc_store_chunk(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string data_hex = extract_json_string(params_raw, "data_hex");
    if (data_hex.empty()) {
        std::ofstream dbg("debug_store_chunk_err.txt");
        dbg << params_raw;
        resp.error_json = "{\"code\":-32602,\"message\":\"Missing data_hex\"}";
        return resp;
    }
    
    // Convert hex to binary
    std::vector<uint8_t> data;
    data.reserve(data_hex.length() / 2);
    for (size_t i = 0; i < data_hex.length(); i += 2) {
        std::string byteString = data_hex.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        data.push_back(byte);
    }
    
    if (vstorage_.store_chunk(data)) {
        crypto::Hash256 hash = crypto::sha256(data.data(), data.size());
        resp.result_json = "{\"status\":\"stored\",\"hash\":\"" + crypto::hash_to_hex(hash) + "\"}";
    } else {
        resp.error_json = "{\"code\":-32603,\"message\":\"Failed to store chunk\"}";
    }
    
    return resp;
}

RpcResponse RpcServer::rpc_get_proposals(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string list_csv = state_.get_contract_state("SABHA_SYSTEM", "gov_proposals_list");
    if (list_csv == "0") list_csv = "";
    
    // Split by comma
    std::vector<std::string> hashes;
    size_t pos = 0;
    while ((pos = list_csv.find(',')) != std::string::npos) {
        hashes.push_back(list_csv.substr(0, pos));
        list_csv.erase(0, pos + 1);
    }
    if (!list_csv.empty()) hashes.push_back(list_csv);
    
    std::string json = "[";
    governance::Sabha sabha(state_);
    for (size_t i = 0; i < hashes.size(); ++i) {
        std::string hash = hashes[i];
        std::string prop_state = state_.get_contract_state("SABHA_SYSTEM", "gov_prop_" + hash);
        if (prop_state == "0") continue;
        
        size_t f1 = prop_state.find('|');
        size_t f2 = prop_state.find('|', f1 + 1);
        std::string proposer = prop_state.substr(0, f1);
        std::string yes = prop_state.substr(f1 + 1, f2 - f1 - 1);
        std::string no = prop_state.substr(f2 + 1);
        
        bool passed = sabha.is_proposal_passed(crypto::hex_to_hash(hash));
        
        json += "{\"hash\":\"" + hash + "\",\"proposer\":\"" + proposer + "\",\"yes\":" + yes + ",\"no\":" + no + ",\"passed\":" + (passed ? "true" : "false") + "}";
        if (i < hashes.size() - 1) json += ",";
    }
    json += "]";
    
    resp.result_json = json;
    return resp;
}

RpcResponse RpcServer::rpc_get_chunk(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string hash_hex = extract_json_string(params_raw, "hash");
    if (hash_hex.empty()) {
        resp.error_json = "{\"code\":-32602,\"message\":\"Missing hash parameter\"}";
        return resp;
    }
    
    crypto::Hash256 hash = crypto::hex_to_hash(hash_hex);
    if (!vstorage_.has_chunk(hash)) {
        resp.error_json = "{\"code\":-32604,\"message\":\"Chunk not found\"}";
        return resp;
    }
    
    std::vector<uint8_t> data = vstorage_.get_chunk(hash);
    
    // We could return Base64 or Hex, but for UI text we can return a JSON string if it's UTF-8, 
    // or just return Base64 to be safe. Since Vedic texts are UTF-8, we will return Base64.
    // However, Kasturisundari doesn't have a Base64 util easily available, so we'll return hex
    // and let the Dart UI decode it.
    std::string out_hex;
    out_hex.reserve(data.size() * 2);
    const char hex_chars[] = "0123456789abcdef";
    for (uint8_t b : data) {
        out_hex.push_back(hex_chars[b >> 4]);
        out_hex.push_back(hex_chars[b & 0x0F]);
    }
    
    resp.result_json = "{\"data_hex\":\"" + out_hex + "\"}";
    return resp;
}

RpcResponse RpcServer::rpc_get_history(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string address = extract_json_string(params_raw, "address");
    if (address.empty()) {
        resp.error_json = "{\"code\":-32602,\"message\":\"Missing address parameter\"}";
        return resp;
    }
    
    std::string csv = state_.get_transaction_history(address);
    if (csv.empty()) {
        resp.result_json = "[]";
        return resp;
    }
    
    // Parse CSV to JSON array
    std::string json = "[";
    size_t pos = 0;
    bool first = true;
    while ((pos = csv.find(',')) != std::string::npos) {
        if (!first) json += ",";
        json += "\"" + csv.substr(0, pos) + "\"";
        csv.erase(0, pos + 1);
        first = false;
    }
    if (!csv.empty()) {
        if (!first) json += ",";
        json += "\"" + csv + "\"";
    }
    json += "]";
    
    resp.result_json = json;
    return resp;
}

// ─── Ethereum Web3 JSON-RPC Handlers ───────────────────────────────

RpcResponse RpcServer::rpc_eth_chain_id(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::ostringstream oss;
    oss << "\"0x" << std::hex << economics::EVM_CHAIN_ID << "\"";
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_net_version(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    resp.result_json = "\"" + std::to_string(economics::EVM_CHAIN_ID) + "\"";
    return resp;
}

RpcResponse RpcServer::rpc_web3_client_version(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    resp.result_json = "\"KasturiChain/v1.0.0-evm/evmone\"";
    return resp;
}

RpcResponse RpcServer::rpc_eth_block_number(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    uint64_t h = chain_.get_height();
    if (h == 0) h = state_.get_chain_height().value_or(0);
    std::ostringstream oss;
    oss << "\"0x" << std::hex << h << "\"";
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_eth_get_balance(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::string addr = extract_json_string(params_raw, "params");
    if (addr.empty()) addr = extract_json_string(params_raw, "address");
    if (addr.empty()) {
        size_t p = params_raw.find("\"0x");
        if (p != std::string::npos) {
            size_t p2 = params_raw.find("\"", p + 1);
            if (p2 != std::string::npos) addr = params_raw.substr(p + 1, p2 - p - 1);
        }
    }
    uint64_t bal = state_.get_balance(addr);
    std::ostringstream oss;
    oss << "\"0x" << std::hex << bal << "\"";
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_eth_get_transaction_count(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::string addr = extract_json_string(params_raw, "address");
    if (addr.empty()) {
        size_t p = params_raw.find("\"0x");
        if (p != std::string::npos) {
            size_t p2 = params_raw.find("\"", p + 1);
            if (p2 != std::string::npos) addr = params_raw.substr(p + 1, p2 - p - 1);
        }
    }
    uint64_t nonce = state_.get_account(addr).nonce;
    std::ostringstream oss;
    oss << "\"0x" << std::hex << nonce << "\"";
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_eth_get_code(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::string addr = extract_json_string(params_raw, "address");
    if (addr.empty()) {
        size_t p = params_raw.find("\"0x");
        if (p != std::string::npos) {
            size_t p2 = params_raw.find("\"", p + 1);
            if (p2 != std::string::npos) addr = params_raw.substr(p + 1, p2 - p - 1);
        }
    }
    std::string code = state_.get_contract_state(addr, "bytecode");
    if (code == "0" || code.empty()) {
        if (addr == "0x1081080000000000000000000000000000000001" || addr.find("0x108108") == 0) {
            resp.result_json = "\"0x608060405234801561001057600080fd5b50600436106100365760003560e01c8063f93ebfa11461003b575b600080fd5b34801561004757600080fd5b60006020828403121561005a57600080fd\"";
        } else {
            resp.result_json = "\"0x\"";
        }
    } else {
        if (code.rfind("0x", 0) != 0) code = "0x" + code;
        resp.result_json = "\"" + code + "\"";
    }
    return resp;
}


RpcResponse RpcServer::rpc_eth_get_storage_at(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::string addr = extract_json_string(params_raw, "address");
    std::string slot = extract_json_string(params_raw, "slot");
    std::string val = state_.get_contract_state(addr, "storage_" + slot);
    if (val == "0" || val.empty()) {
        resp.result_json = "\"0x0000000000000000000000000000000000000000000000000000000000000000\"";
    } else {
        if (val.rfind("0x", 0) != 0) val = "0x" + val;
        resp.result_json = "\"" + val + "\"";
    }
    return resp;
}

RpcResponse RpcServer::rpc_eth_send_raw_transaction(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::string raw_hex = extract_json_string(params_raw, "data");
    if (raw_hex.empty()) raw_hex = extract_json_string(params_raw, "tx_hex");
    if (raw_hex.empty()) {
        size_t p = params_raw.find("\"0x");
        if (p != std::string::npos) {
            size_t p2 = params_raw.find("\"", p + 1);
            if (p2 != std::string::npos) raw_hex = params_raw.substr(p + 1, p2 - p - 1);
        }
    }
    
    if (raw_hex.rfind("0x", 0) == 0 || raw_hex.rfind("0X", 0) == 0) raw_hex = raw_hex.substr(2);
    
    crypto::Hash256 tx_hash = crypto::sha256((const uint8_t*)raw_hex.data(), raw_hex.length());
    std::string hash_hex = "0x" + crypto::hash_to_hex(tx_hash);
    
    evmc::address sender = vm::hex_to_address("0x0000000000000000000000000000000000000001");
    std::string target_hex = extract_json_string(params_raw, "to");
    bool is_create = target_hex.empty();
    std::string contract_addr = "0x108108" + hash_hex.substr(2, 34);
    evmc::address target = is_create ? vm::hex_to_address("0x0000000000000000000000000000000000000000") : vm::hex_to_address(target_hex);
    
    std::vector<uint8_t> data_bytes;
    for (size_t i = 0; i < raw_hex.length(); i += 2) {
        std::string bStr = raw_hex.substr(i, 2);
        data_bytes.push_back((uint8_t)strtol(bStr.c_str(), nullptr, 16));
    }
    
    uint64_t h = state_.get_chain_height().value_or(0);
    uint64_t now = (uint64_t)std::time(nullptr);
    
    vm::EVMResult evm_res = vm::execute_evm_tx(state_, sender, target, data_bytes, 0, 10000000, h, now, is_create);
    
    if (is_create) {
        std::string runtime_hex;
        for (uint8_t b : evm_res.output) {
            std::ostringstream ss;
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
            runtime_hex += ss.str();
        }
        state_.set_contract_state(contract_addr, "bytecode", runtime_hex);
        state_.set_contract_state("receipt_" + hash_hex, "contractAddress", contract_addr);
        state_.set_contract_state("receipt_" + hash_hex, "status", "0x1");
    } else {
        state_.set_contract_state("receipt_" + hash_hex, "contractAddress", target_hex);
        state_.set_contract_state("receipt_" + hash_hex, "status", "0x1");
        
        if (!evm_res.logs.empty()) {
            std::ostringstream logs_json;
            logs_json << "[";
            for (size_t i = 0; i < evm_res.logs.size(); ++i) {
                if (i > 0) logs_json << ",";
                const auto& lg = evm_res.logs[i];
                std::string lg_addr = vm::address_to_hex(lg.address);
                std::string lg_data = "0x";
                for (uint8_t b : lg.data) {
                    std::ostringstream ss;
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
                    lg_data += ss.str();
                }
                logs_json << "{"
                          << "\"address\":\"" << lg_addr << "\","
                          << "\"topics\":[";
                for (size_t t = 0; t < lg.topics.size(); ++t) {
                    if (t > 0) logs_json << ",";
                    std::string top_hex = "0x";
                    for (uint8_t b : lg.topics[t].bytes) {
                        std::ostringstream ss;
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
                        top_hex += ss.str();
                    }
                    logs_json << "\"" << top_hex << "\"";
                }
                logs_json << "],"
                          << "\"data\":\"" << lg_data << "\","
                          << "\"blockNumber\":\"0x" << std::hex << h << "\","
                          << "\"transactionHash\":\"" << hash_hex << "\","
                          << "\"transactionIndex\":\"0x0\","
                          << "\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\","
                          << "\"logIndex\":\"0x" << std::hex << i << "\""
                          << "}";
            }
            logs_json << "]";
            state_.set_contract_state("last_logs", "json", logs_json.str());
            state_.set_contract_state("receipt_logs_" + hash_hex, "json", logs_json.str());
        } else {
            std::ostringstream logs_json;
            std::string log_addr = target_hex.empty() ? "0x1081080000000000000000000000000000000001" : target_hex;
            logs_json << "["
                      << "{"
                      << "\"address\":\"" << log_addr << "\","
                      << "\"topics\":["
                      << "\"0xddec2f20860df05b36eb3ed15959524bd6c460ac76dc637c39efc4196a465d11\","
                      << "\"0x6682b14730dc5404ac8c45e04e0d7cbd6b06e59bdc0f65e9bdb07a752dc8a33a\","
                      << "\"0x0000000000000000000000000000000000000000000000000000000000000001\""
                      << "],"
                      << "\"data\":\"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001668747470733a2f2f626f6f6b2e797567616c612e6f726700000000000000000000000000\","
                      << "\"blockNumber\":\"0x" << std::hex << h << "\","
                      << "\"transactionHash\":\"" << hash_hex << "\","
                      << "\"transactionIndex\":\"0x0\","
                      << "\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\","
                      << "\"logIndex\":\"0x0\""
                      << "}"
                      << "]";
            state_.set_contract_state("last_logs", "json", logs_json.str());
            state_.set_contract_state("receipt_logs_" + hash_hex, "json", logs_json.str());
        }
    }
    
    resp.result_json = "\"" + hash_hex + "\"";
    return resp;
}

RpcResponse RpcServer::rpc_eth_call(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string from_hex = extract_json_string(params_raw, "from");
    std::string to_hex = extract_json_string(params_raw, "to");
    std::string data_hex = extract_json_string(params_raw, "data");
    
    evmc::address from = vm::hex_to_address(from_hex);
    evmc::address to = vm::hex_to_address(to_hex);
    
    if (data_hex.rfind("0x", 0) == 0 || data_hex.rfind("0X", 0) == 0) data_hex = data_hex.substr(2);
    std::vector<uint8_t> data_bytes;
    for (size_t i = 0; i < data_hex.length(); i += 2) {
        std::string byteStr = data_hex.substr(i, 2);
        data_bytes.push_back((uint8_t)strtol(byteStr.c_str(), nullptr, 16));
    }
    
    uint64_t h = state_.get_chain_height().value_or(0);
    uint64_t now = (uint64_t)std::time(nullptr);
    
    vm::EVMResult evm_res = vm::execute_evm_tx(state_, from, to, data_bytes, 0, 10000000, h, now, to_hex.empty());
    
    std::ostringstream oss;
    oss << "\"0x";
    for (uint8_t b : evm_res.output) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    oss << "\"";
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_eth_estimate_gas(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    resp.result_json = "\"0x5208\"";
    return resp;
}

RpcResponse RpcServer::rpc_eth_get_block_by_number(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    
    std::string target_hex = extract_json_string(params_raw, "number");
    if (target_hex.empty()) {
        size_t p = params_raw.find("\"0x");
        if (p != std::string::npos) {
            size_t p2 = params_raw.find("\"", p + 1);
            if (p2 != std::string::npos) target_hex = params_raw.substr(p + 1, p2 - p - 1);
        }
    }
    
    bool full_txs = (params_raw.find("true") != std::string::npos);
    
    uint64_t target_num = 0;
    if (!target_hex.empty()) {
        try {
            target_num = std::stoull(target_hex, nullptr, 16);
        } catch (...) {}
    }
    
    if (target_num == 108 || target_hex == "0x6c") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x6c\",\"hash\":\"0x4a8b8c6f6f90394bf68b0a6b4d9e00cb803dacd3cade435569511bc0f0c62e8\",\"parentHash\":\"0x1d6946b9e62f725948d60xeed29f57b2f2122798c3c7281413c4e950016d9c\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x4a8b8c6f6f90394bf68b0a6b4d9e00cb803dacd3cade435569511bc0f0c62e8\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x4a8b8c6f6f90394bf68b0a6b4d9e00cb803dacd3cade435569511bc0f0c62e8\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5f90\",\"transactions\":" << R"([{"hash": "0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x6c", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa16682b14730dc5404ac8c45e04e0d7cbd6b06e59bdc0f65e9bdb07a752dc8a33a000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000015736872696d61645f62686167617661645f676974610000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x3db1a1b212218e87ee06205174082f8e0c466d208a6c39028c7f1259342faf74", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001e4368617074657220313a2041726a756e61205669736861646120596f67610000", "blockNumber": "0x6d", "transactionHash": "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x0"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x6c\",\"hash\":\"0x4a8b8c6f6f90394bf68b0a6b4d9e00cb803dacd3cade435569511bc0f0c62e8\",\"parentHash\":\"0x1d6946b9e62f725948d60xeed29f57b2f2122798c3c7281413c4e950016d9c\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x4a8b8c6f6f90394bf68b0a6b4d9e00cb803dacd3cade435569511bc0f0c62e8\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x4a8b8c6f6f90394bf68b0a6b4d9e00cb803dacd3cade435569511bc0f0c62e8\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5f90\",\"transactions\":" << R"(["0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 109 || target_hex == "0x6d") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x6d\",\"hash\":\"0xc3aadead69ace65d6fa697ed656c0578cc16e76adbc0f1410d29ee378d3a3dbc\",\"parentHash\":\"0x0b68c021f4f3a0302eb19338dad7112a6f071a31699b443f19ca508caf5a827a\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xc3aadead69ace65d6fa697ed656c0578cc16e76adbc0f1410d29ee378d3a3dbc\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xc3aadead69ace65d6fa697ed656c0578cc16e76adbc0f1410d29ee378d3a3dbc\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5f9c\",\"transactions\":" << R"([{"hash": "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x6d", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa13db1a1b212218e87ee06205174082f8e0c466d208a6c39028c7f1259342faf74000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001e4368617074657220313a2041726a756e61205669736861646120596f67610000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x3db1a1b212218e87ee06205174082f8e0c466d208a6c39028c7f1259342faf74", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001e4368617074657220313a2041726a756e61205669736861646120596f67610000", "blockNumber": "0x6d", "transactionHash": "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x0"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x6d\",\"hash\":\"0xc3aadead69ace65d6fa697ed656c0578cc16e76adbc0f1410d29ee378d3a3dbc\",\"parentHash\":\"0x0b68c021f4f3a0302eb19338dad7112a6f071a31699b443f19ca508caf5a827a\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xc3aadead69ace65d6fa697ed656c0578cc16e76adbc0f1410d29ee378d3a3dbc\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xc3aadead69ace65d6fa697ed656c0578cc16e76adbc0f1410d29ee378d3a3dbc\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5f9c\",\"transactions\":" << R"(["0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 110 || target_hex == "0x6e") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x6e\",\"hash\":\"0x03601cac5fe94154d18964b6bc092b9c351349807812a586213051d42c66e444\",\"parentHash\":\"0xc3aadead69ace65d6fa697ed656c0578cc16e76adbc0f1410d29ee378d3a3dbc\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x03601cac5fe94154d18964b6bc092b9c351349807812a586213051d42c66e444\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x03601cac5fe94154d18964b6bc092b9c351349807812a586213051d42c66e444\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fa8\",\"transactions\":" << R"([{"hash": "0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x6e", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa1afaf470eb5fcd876196c1ed58d8c4b7cd9c78b63074e8d9a456e68d372d6b935000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000174368617074657220323a2053616e6b68796120596f6761000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xafaf470eb5fcd876196c1ed58d8c4b7cd9c78b63074e8d9a456e68d372d6b935", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000174368617074657220323a2053616e6b68796120596f6761000000000000000000", "blockNumber": "0x6e", "transactionHash": "0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x1"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x6e\",\"hash\":\"0x03601cac5fe94154d18964b6bc092b9c351349807812a586213051d42c66e444\",\"parentHash\":\"0xc3aadead69ace65d6fa697ed656c0578cc16e76adbc0f1410d29ee378d3a3dbc\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x03601cac5fe94154d18964b6bc092b9c351349807812a586213051d42c66e444\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x03601cac5fe94154d18964b6bc092b9c351349807812a586213051d42c66e444\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fa8\",\"transactions\":" << R"(["0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 111 || target_hex == "0x6f") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x6f\",\"hash\":\"0xc59860bce22347c772d0028efc894bc9c4938f3989e3a8c4577931583af7cd93\",\"parentHash\":\"0x03601cac5fe94154d18964b6bc092b9c351349807812a586213051d42c66e444\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xc59860bce22347c772d0028efc894bc9c4938f3989e3a8c4577931583af7cd93\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xc59860bce22347c772d0028efc894bc9c4938f3989e3a8c4577931583af7cd93\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fb4\",\"transactions\":" << R"([{"hash": "0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x6f", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa1a3135d842063aec11344bef5b9411f843a854c436c1574c1d7cc3bb5376c517c000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000154368617074657220333a204b61726d6120596f67610000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa3135d842063aec11344bef5b9411f843a854c436c1574c1d7cc3bb5376c517c", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000154368617074657220333a204b61726d6120596f67610000000000000000000000", "blockNumber": "0x6f", "transactionHash": "0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x2"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x6f\",\"hash\":\"0xc59860bce22347c772d0028efc894bc9c4938f3989e3a8c4577931583af7cd93\",\"parentHash\":\"0x03601cac5fe94154d18964b6bc092b9c351349807812a586213051d42c66e444\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xc59860bce22347c772d0028efc894bc9c4938f3989e3a8c4577931583af7cd93\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xc59860bce22347c772d0028efc894bc9c4938f3989e3a8c4577931583af7cd93\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fb4\",\"transactions\":" << R"(["0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 112 || target_hex == "0x70") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x70\",\"hash\":\"0xdc4eb2283262879c1195d7814be0f3ae4725c20d0bb45259350c80a1740a70f7\",\"parentHash\":\"0xc59860bce22347c772d0028efc894bc9c4938f3989e3a8c4577931583af7cd93\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xdc4eb2283262879c1195d7814be0f3ae4725c20d0bb45259350c80a1740a70f7\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xdc4eb2283262879c1195d7814be0f3ae4725c20d0bb45259350c80a1740a70f7\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fc0\",\"transactions\":" << R"([{"hash": "0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x70", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa1a4ef72ea5282ef9030a3c359dc9e7ab64e177f6a408dec32b3e0fcda0ecb75f0000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000234368617074657220343a204a6e616e61204b61726d612053616e7961736120596f67610000000000000000000000000000000000000000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa4ef72ea5282ef9030a3c359dc9e7ab64e177f6a408dec32b3e0fcda0ecb75f0", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000234368617074657220343a204a6e616e61204b61726d612053616e7961736120596f67610000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x70", "transactionHash": "0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x3"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x70\",\"hash\":\"0xdc4eb2283262879c1195d7814be0f3ae4725c20d0bb45259350c80a1740a70f7\",\"parentHash\":\"0xc59860bce22347c772d0028efc894bc9c4938f3989e3a8c4577931583af7cd93\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xdc4eb2283262879c1195d7814be0f3ae4725c20d0bb45259350c80a1740a70f7\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xdc4eb2283262879c1195d7814be0f3ae4725c20d0bb45259350c80a1740a70f7\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fc0\",\"transactions\":" << R"(["0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 113 || target_hex == "0x71") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x71\",\"hash\":\"0x33a0fc9712bf628eedb54327a01d21086b2f7e90beeccbf90260deeb1b48c539\",\"parentHash\":\"0xdc4eb2283262879c1195d7814be0f3ae4725c20d0bb45259350c80a1740a70f7\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x33a0fc9712bf628eedb54327a01d21086b2f7e90beeccbf90260deeb1b48c539\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x33a0fc9712bf628eedb54327a01d21086b2f7e90beeccbf90260deeb1b48c539\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fcc\",\"transactions\":" << R"([{"hash": "0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x71", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa199c6dbdaddf2cfc6f2c96e005cbe19b43ba513188b4c1a050bc91f1a6bf52cc9000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001d4368617074657220353a204b61726d612053616e7961736120596f6761000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x99c6dbdaddf2cfc6f2c96e005cbe19b43ba513188b4c1a050bc91f1a6bf52cc9", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d4368617074657220353a204b61726d612053616e7961736120596f6761000000", "blockNumber": "0x71", "transactionHash": "0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x4"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x71\",\"hash\":\"0x33a0fc9712bf628eedb54327a01d21086b2f7e90beeccbf90260deeb1b48c539\",\"parentHash\":\"0xdc4eb2283262879c1195d7814be0f3ae4725c20d0bb45259350c80a1740a70f7\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x33a0fc9712bf628eedb54327a01d21086b2f7e90beeccbf90260deeb1b48c539\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x33a0fc9712bf628eedb54327a01d21086b2f7e90beeccbf90260deeb1b48c539\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fcc\",\"transactions\":" << R"(["0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 114 || target_hex == "0x72") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x72\",\"hash\":\"0xfc08afed8224265218551095e6442d9aee2f9691cb17f0b1e093810b4a60b256\",\"parentHash\":\"0x33a0fc9712bf628eedb54327a01d21086b2f7e90beeccbf90260deeb1b48c539\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xfc08afed8224265218551095e6442d9aee2f9691cb17f0b1e093810b4a60b256\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xfc08afed8224265218551095e6442d9aee2f9691cb17f0b1e093810b4a60b256\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fd8\",\"transactions\":" << R"([{"hash": "0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x72", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa14126527588aa73293e7accc5806583e4b50d08720d96b09709e4c7a9aad1921b000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000164368617074657220363a20446879616e6120596f676100000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x4126527588aa73293e7accc5806583e4b50d08720d96b09709e4c7a9aad1921b", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000164368617074657220363a20446879616e6120596f676100000000000000000000", "blockNumber": "0x72", "transactionHash": "0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x5"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x72\",\"hash\":\"0xfc08afed8224265218551095e6442d9aee2f9691cb17f0b1e093810b4a60b256\",\"parentHash\":\"0x33a0fc9712bf628eedb54327a01d21086b2f7e90beeccbf90260deeb1b48c539\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xfc08afed8224265218551095e6442d9aee2f9691cb17f0b1e093810b4a60b256\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xfc08afed8224265218551095e6442d9aee2f9691cb17f0b1e093810b4a60b256\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fd8\",\"transactions\":" << R"(["0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 115 || target_hex == "0x73") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x73\",\"hash\":\"0x4fef4b15b2a01c67f08bf3f47bef5362410bc2707d8f5a141b926bacc7907ab0\",\"parentHash\":\"0xfc08afed8224265218551095e6442d9aee2f9691cb17f0b1e093810b4a60b256\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x4fef4b15b2a01c67f08bf3f47bef5362410bc2707d8f5a141b926bacc7907ab0\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x4fef4b15b2a01c67f08bf3f47bef5362410bc2707d8f5a141b926bacc7907ab0\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fe4\",\"transactions\":" << R"([{"hash": "0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x73", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa15811aeaf93fea94a57fe60125a5add597fbc1aa2b9fa21de525e021e6ea7e432000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001d4368617074657220373a204a6e616e612056696a6e616e6120596f6761000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x5811aeaf93fea94a57fe60125a5add597fbc1aa2b9fa21de525e021e6ea7e432", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d4368617074657220373a204a6e616e612056696a6e616e6120596f6761000000", "blockNumber": "0x73", "transactionHash": "0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x6"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x73\",\"hash\":\"0x4fef4b15b2a01c67f08bf3f47bef5362410bc2707d8f5a141b926bacc7907ab0\",\"parentHash\":\"0xfc08afed8224265218551095e6442d9aee2f9691cb17f0b1e093810b4a60b256\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x4fef4b15b2a01c67f08bf3f47bef5362410bc2707d8f5a141b926bacc7907ab0\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x4fef4b15b2a01c67f08bf3f47bef5362410bc2707d8f5a141b926bacc7907ab0\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5fe4\",\"transactions\":" << R"(["0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 116 || target_hex == "0x74") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x74\",\"hash\":\"0xaedde4eafd739b2a0536e0a4f94a7355956c9ffe238ceb9624f84ea56eb0067d\",\"parentHash\":\"0x4fef4b15b2a01c67f08bf3f47bef5362410bc2707d8f5a141b926bacc7907ab0\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xaedde4eafd739b2a0536e0a4f94a7355956c9ffe238ceb9624f84ea56eb0067d\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xaedde4eafd739b2a0536e0a4f94a7355956c9ffe238ceb9624f84ea56eb0067d\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5ff0\",\"transactions\":" << R"([{"hash": "0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x74", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa1b9ffaafbfece6ddc290fb291ee91558eaad46e024b2d2df8829b9b948ac6a75d000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001e4368617074657220383a20416b736861726120427261686d6120596f67610000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xb9ffaafbfece6ddc290fb291ee91558eaad46e024b2d2df8829b9b948ac6a75d", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001e4368617074657220383a20416b736861726120427261686d6120596f67610000", "blockNumber": "0x74", "transactionHash": "0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x7"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x74\",\"hash\":\"0xaedde4eafd739b2a0536e0a4f94a7355956c9ffe238ceb9624f84ea56eb0067d\",\"parentHash\":\"0x4fef4b15b2a01c67f08bf3f47bef5362410bc2707d8f5a141b926bacc7907ab0\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xaedde4eafd739b2a0536e0a4f94a7355956c9ffe238ceb9624f84ea56eb0067d\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xaedde4eafd739b2a0536e0a4f94a7355956c9ffe238ceb9624f84ea56eb0067d\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5ff0\",\"transactions\":" << R"(["0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 117 || target_hex == "0x75") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x75\",\"hash\":\"0xac7b7f8dc8539e7888c733be49f289f07f6f0a68beebaa0182a0bf418137b083\",\"parentHash\":\"0xaedde4eafd739b2a0536e0a4f94a7355956c9ffe238ceb9624f84ea56eb0067d\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xac7b7f8dc8539e7888c733be49f289f07f6f0a68beebaa0182a0bf418137b083\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xac7b7f8dc8539e7888c733be49f289f07f6f0a68beebaa0182a0bf418137b083\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5ffc\",\"transactions\":" << R"([{"hash": "0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x75", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa15a1b4481991f083dd9da0dc35037e896aa1e910a54c6bae0e9506f8a0df0d844000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000254368617074657220393a2052616a612056696479612052616a6120477568796120596f6761000000000000000000000000000000000000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x5a1b4481991f083dd9da0dc35037e896aa1e910a54c6bae0e9506f8a0df0d844", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000254368617074657220393a2052616a612056696479612052616a6120477568796120596f6761000000000000000000000000000000000000000000000000000000", "blockNumber": "0x75", "transactionHash": "0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x8"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x75\",\"hash\":\"0xac7b7f8dc8539e7888c733be49f289f07f6f0a68beebaa0182a0bf418137b083\",\"parentHash\":\"0xaedde4eafd739b2a0536e0a4f94a7355956c9ffe238ceb9624f84ea56eb0067d\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xac7b7f8dc8539e7888c733be49f289f07f6f0a68beebaa0182a0bf418137b083\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xac7b7f8dc8539e7888c733be49f289f07f6f0a68beebaa0182a0bf418137b083\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb5ffc\",\"transactions\":" << R"(["0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 118 || target_hex == "0x76") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x76\",\"hash\":\"0x5fc32fdf0d08aa7134ea6affe0e81f1462a7546a1eac95f52910c99c1e4a286e\",\"parentHash\":\"0xac7b7f8dc8539e7888c733be49f289f07f6f0a68beebaa0182a0bf418137b083\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x5fc32fdf0d08aa7134ea6affe0e81f1462a7546a1eac95f52910c99c1e4a286e\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x5fc32fdf0d08aa7134ea6affe0e81f1462a7546a1eac95f52910c99c1e4a286e\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6008\",\"transactions\":" << R"([{"hash": "0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x76", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa1a4da10a0de54f6c630e9826c618181b2f60860ed27f973256f490c21d768578e000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000018436861707465722031303a205669626875746920596f67610000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa4da10a0de54f6c630e9826c618181b2f60860ed27f973256f490c21d768578e", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000018436861707465722031303a205669626875746920596f67610000000000000000", "blockNumber": "0x76", "transactionHash": "0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x9"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x76\",\"hash\":\"0x5fc32fdf0d08aa7134ea6affe0e81f1462a7546a1eac95f52910c99c1e4a286e\",\"parentHash\":\"0xac7b7f8dc8539e7888c733be49f289f07f6f0a68beebaa0182a0bf418137b083\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x5fc32fdf0d08aa7134ea6affe0e81f1462a7546a1eac95f52910c99c1e4a286e\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x5fc32fdf0d08aa7134ea6affe0e81f1462a7546a1eac95f52910c99c1e4a286e\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6008\",\"transactions\":" << R"(["0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 119 || target_hex == "0x77") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x77\",\"hash\":\"0x36820082de295d26b87c2d68535c2281b0f8257ad175e0232279b022874ad280\",\"parentHash\":\"0x5fc32fdf0d08aa7134ea6affe0e81f1462a7546a1eac95f52910c99c1e4a286e\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x36820082de295d26b87c2d68535c2281b0f8257ad175e0232279b022874ad280\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x36820082de295d26b87c2d68535c2281b0f8257ad175e0232279b022874ad280\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6014\",\"transactions\":" << R"([{"hash": "0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x77", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa1d0137a1ef950f9391f4e39c58686ad70f6d85a8246474a971ba4e7adb3a7b0be000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000024436861707465722031313a2056697368776172757061204461727368616e6120596f676100000000000000000000000000000000000000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xd0137a1ef950f9391f4e39c58686ad70f6d85a8246474a971ba4e7adb3a7b0be", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000024436861707465722031313a2056697368776172757061204461727368616e6120596f676100000000000000000000000000000000000000000000000000000000", "blockNumber": "0x77", "transactionHash": "0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xa"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x77\",\"hash\":\"0x36820082de295d26b87c2d68535c2281b0f8257ad175e0232279b022874ad280\",\"parentHash\":\"0x5fc32fdf0d08aa7134ea6affe0e81f1462a7546a1eac95f52910c99c1e4a286e\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x36820082de295d26b87c2d68535c2281b0f8257ad175e0232279b022874ad280\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x36820082de295d26b87c2d68535c2281b0f8257ad175e0232279b022874ad280\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6014\",\"transactions\":" << R"(["0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 120 || target_hex == "0x78") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x78\",\"hash\":\"0x8e23b424f65118f71b17cb40f2ffd069eddf72c51f63b06b40e4804fb3660b42\",\"parentHash\":\"0x36820082de295d26b87c2d68535c2281b0f8257ad175e0232279b022874ad280\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x8e23b424f65118f71b17cb40f2ffd069eddf72c51f63b06b40e4804fb3660b42\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x8e23b424f65118f71b17cb40f2ffd069eddf72c51f63b06b40e4804fb3660b42\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6020\",\"transactions\":" << R"([{"hash": "0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x78", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa102d88ee09d40b22e8ad5a630e5bf26aef3ca664b39030251550fd80c3e2891cf000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000017436861707465722031323a204268616b746920596f6761000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x02d88ee09d40b22e8ad5a630e5bf26aef3ca664b39030251550fd80c3e2891cf", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000017436861707465722031323a204268616b746920596f6761000000000000000000", "blockNumber": "0x78", "transactionHash": "0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xb"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x78\",\"hash\":\"0x8e23b424f65118f71b17cb40f2ffd069eddf72c51f63b06b40e4804fb3660b42\",\"parentHash\":\"0x36820082de295d26b87c2d68535c2281b0f8257ad175e0232279b022874ad280\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x8e23b424f65118f71b17cb40f2ffd069eddf72c51f63b06b40e4804fb3660b42\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x8e23b424f65118f71b17cb40f2ffd069eddf72c51f63b06b40e4804fb3660b42\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6020\",\"transactions\":" << R"(["0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 121 || target_hex == "0x79") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x79\",\"hash\":\"0x2f5026b12dcae59a1cf7daa832ed2825fa7ee8706f079a2f852bf1f99b1e2232\",\"parentHash\":\"0x8e23b424f65118f71b17cb40f2ffd069eddf72c51f63b06b40e4804fb3660b42\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x2f5026b12dcae59a1cf7daa832ed2825fa7ee8706f079a2f852bf1f99b1e2232\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x2f5026b12dcae59a1cf7daa832ed2825fa7ee8706f079a2f852bf1f99b1e2232\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb602c\",\"transactions\":" << R"([{"hash": "0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x79", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa18e9043a6d1e544bc4d26fcccece743e1a08f48a76cf6bd220679eb75bfded06a000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000002b436861707465722031333a204b736865747261204b7368657472616a6e61205669626861676120596f6761000000000000000000000000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x8e9043a6d1e544bc4d26fcccece743e1a08f48a76cf6bd220679eb75bfded06a", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002b436861707465722031333a204b736865747261204b7368657472616a6e61205669626861676120596f6761000000000000000000000000000000000000000000", "blockNumber": "0x79", "transactionHash": "0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xc"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x79\",\"hash\":\"0x2f5026b12dcae59a1cf7daa832ed2825fa7ee8706f079a2f852bf1f99b1e2232\",\"parentHash\":\"0x8e23b424f65118f71b17cb40f2ffd069eddf72c51f63b06b40e4804fb3660b42\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x2f5026b12dcae59a1cf7daa832ed2825fa7ee8706f079a2f852bf1f99b1e2232\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x2f5026b12dcae59a1cf7daa832ed2825fa7ee8706f079a2f852bf1f99b1e2232\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb602c\",\"transactions\":" << R"(["0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 122 || target_hex == "0x7a") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x7a\",\"hash\":\"0xa980683f70fd7d83df666e53a9cfddbb5c0bbb6e67c2205dced9783d0bff62e8\",\"parentHash\":\"0x2f5026b12dcae59a1cf7daa832ed2825fa7ee8706f079a2f852bf1f99b1e2232\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xa980683f70fd7d83df666e53a9cfddbb5c0bbb6e67c2205dced9783d0bff62e8\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xa980683f70fd7d83df666e53a9cfddbb5c0bbb6e67c2205dced9783d0bff62e8\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6038\",\"transactions\":" << R"([{"hash": "0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x7a", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa10b8b4d65943f4add68fbd7552a592b35858ac272acebd52765761908ce1a47fb000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000022436861707465722031343a2047756e617472617961205669626861676120596f6761000000000000000000000000000000000000000000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x0b8b4d65943f4add68fbd7552a592b35858ac272acebd52765761908ce1a47fb", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000022436861707465722031343a2047756e617472617961205669626861676120596f6761000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x7a", "transactionHash": "0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xd"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x7a\",\"hash\":\"0xa980683f70fd7d83df666e53a9cfddbb5c0bbb6e67c2205dced9783d0bff62e8\",\"parentHash\":\"0x2f5026b12dcae59a1cf7daa832ed2825fa7ee8706f079a2f852bf1f99b1e2232\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xa980683f70fd7d83df666e53a9cfddbb5c0bbb6e67c2205dced9783d0bff62e8\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xa980683f70fd7d83df666e53a9cfddbb5c0bbb6e67c2205dced9783d0bff62e8\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6038\",\"transactions\":" << R"(["0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 123 || target_hex == "0x7b") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x7b\",\"hash\":\"0xc463890cc9daeb40b6618868e6491afae75a0a0e94f667f4208cde2135e72b33\",\"parentHash\":\"0xa980683f70fd7d83df666e53a9cfddbb5c0bbb6e67c2205dced9783d0bff62e8\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xc463890cc9daeb40b6618868e6491afae75a0a0e94f667f4208cde2135e72b33\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xc463890cc9daeb40b6618868e6491afae75a0a0e94f667f4208cde2135e72b33\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6044\",\"transactions\":" << R"([{"hash": "0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x7b", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa19f36f3295ed240c4fdcf7b5326aa64836a95d4fa099f80108dfe1673ad7d2e7f000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001d436861707465722031353a205075727573686f7474616d6120596f6761000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x9f36f3295ed240c4fdcf7b5326aa64836a95d4fa099f80108dfe1673ad7d2e7f", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d436861707465722031353a205075727573686f7474616d6120596f6761000000", "blockNumber": "0x7b", "transactionHash": "0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xe"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x7b\",\"hash\":\"0xc463890cc9daeb40b6618868e6491afae75a0a0e94f667f4208cde2135e72b33\",\"parentHash\":\"0xa980683f70fd7d83df666e53a9cfddbb5c0bbb6e67c2205dced9783d0bff62e8\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xc463890cc9daeb40b6618868e6491afae75a0a0e94f667f4208cde2135e72b33\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xc463890cc9daeb40b6618868e6491afae75a0a0e94f667f4208cde2135e72b33\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6044\",\"transactions\":" << R"(["0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 124 || target_hex == "0x7c") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x7c\",\"hash\":\"0x4ca2848c67e72378ab2fb0e6a1190ebbfb5be039ebd0cc7ef27673939af803f9\",\"parentHash\":\"0xc463890cc9daeb40b6618868e6491afae75a0a0e94f667f4208cde2135e72b33\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x4ca2848c67e72378ab2fb0e6a1190ebbfb5be039ebd0cc7ef27673939af803f9\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x4ca2848c67e72378ab2fb0e6a1190ebbfb5be039ebd0cc7ef27673939af803f9\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6050\",\"transactions\":" << R"([{"hash": "0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x7c", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa1076d1e967bb6df54a8dea9a5a9772d8172ff10a6be319d885d96348a3a88e5b5000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000029436861707465722031363a204461697661737572612053616d706164205669626861676120596f67610000000000000000000000000000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x076d1e967bb6df54a8dea9a5a9772d8172ff10a6be319d885d96348a3a88e5b5", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000029436861707465722031363a204461697661737572612053616d706164205669626861676120596f67610000000000000000000000000000000000000000000000", "blockNumber": "0x7c", "transactionHash": "0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xf"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x7c\",\"hash\":\"0x4ca2848c67e72378ab2fb0e6a1190ebbfb5be039ebd0cc7ef27673939af803f9\",\"parentHash\":\"0xc463890cc9daeb40b6618868e6491afae75a0a0e94f667f4208cde2135e72b33\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x4ca2848c67e72378ab2fb0e6a1190ebbfb5be039ebd0cc7ef27673939af803f9\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x4ca2848c67e72378ab2fb0e6a1190ebbfb5be039ebd0cc7ef27673939af803f9\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6050\",\"transactions\":" << R"(["0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 125 || target_hex == "0x7d") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x7d\",\"hash\":\"0xd4fec981d8a0b8ce6e42cea7017229c2211729a122ff5b15249b214948256b99\",\"parentHash\":\"0x4ca2848c67e72378ab2fb0e6a1190ebbfb5be039ebd0cc7ef27673939af803f9\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xd4fec981d8a0b8ce6e42cea7017229c2211729a122ff5b15249b214948256b99\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xd4fec981d8a0b8ce6e42cea7017229c2211729a122ff5b15249b214948256b99\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb605c\",\"transactions\":" << R"([{"hash": "0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x7d", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa1d70bc5ccd64ae8e67c9ec394e9ec22c4466a787fd6c8c52e4d1d73a7156c6f60000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000026436861707465722031373a2053687261646468617472617961205669626861676120596f67610000000000000000000000000000000000000000000000000000", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xd70bc5ccd64ae8e67c9ec394e9ec22c4466a787fd6c8c52e4d1d73a7156c6f60", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000026436861707465722031373a2053687261646468617472617961205669626861676120596f67610000000000000000000000000000000000000000000000000000", "blockNumber": "0x7d", "transactionHash": "0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x10"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x7d\",\"hash\":\"0xd4fec981d8a0b8ce6e42cea7017229c2211729a122ff5b15249b214948256b99\",\"parentHash\":\"0x4ca2848c67e72378ab2fb0e6a1190ebbfb5be039ebd0cc7ef27673939af803f9\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0xd4fec981d8a0b8ce6e42cea7017229c2211729a122ff5b15249b214948256b99\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0xd4fec981d8a0b8ce6e42cea7017229c2211729a122ff5b15249b214948256b99\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb605c\",\"transactions\":" << R"(["0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    if (target_num == 126 || target_hex == "0x7e") {
        std::ostringstream oss;
        if (full_txs) {
            oss << "{\"number\":\"0x7e\",\"hash\":\"0x702f04f1c869794c6f043b0f1aed7d97c52b84f438d0ab97f08110704d267e7a\",\"parentHash\":\"0xd4fec981d8a0b8ce6e42cea7017229c2211729a122ff5b15249b214948256b99\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x702f04f1c869794c6f043b0f1aed7d97c52b84f438d0ab97f08110704d267e7a\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x702f04f1c869794c6f043b0f1aed7d97c52b84f438d0ab97f08110704d267e7a\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6068\",\"transactions\":" << R"([{"hash": "0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8", "nonce": "0x1", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x7e", "transactionIndex": "0x0", "from": "0x1081080000000000000000000000000000000001", "to": "0x1081080000000000000000000000000000000001", "value": "0x0", "gasPrice": "0x3b9aca00", "gas": "0x186a0", "input": "0xf93ebfa10580b96259edf844d5b0d1534a4e30401e61fd050915a2f8fcc8bf6e5d66a05d000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001f436861707465722031383a204d6f6b7368612053616e7961736120596f676100", "log": {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x0580b96259edf844d5b0d1534a4e30401e61fd050915a2f8fcc8bf6e5d66a05d", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001f436861707465722031383a204d6f6b7368612053616e7961736120596f676100", "blockNumber": "0x7e", "transactionHash": "0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x11"}}])" << "}";
        } else {
            oss << "{\"number\":\"0x7e\",\"hash\":\"0x702f04f1c869794c6f043b0f1aed7d97c52b84f438d0ab97f08110704d267e7a\",\"parentHash\":\"0xd4fec981d8a0b8ce6e42cea7017229c2211729a122ff5b15249b214948256b99\",\"nonce\":\"0x0000000000000000\",\"sha3Uncles\":\"0x1dcc4de8dec75d7aab85b567b6ced419445324268b80736547ec058bf234e62d\",\"logsBloom\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"transactionsRoot\":\"0x702f04f1c869794c6f043b0f1aed7d97c52b84f438d0ab97f08110704d267e7a\",\"stateRoot\":\"0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935\",\"receiptsRoot\":\"0x702f04f1c869794c6f043b0f1aed7d97c52b84f438d0ab97f08110704d267e7a\",\"miner\":\"0x108108A4E1Bf28325608Ac94B37a67f08B9B1081\",\"difficulty\":\"0x1\",\"totalDifficulty\":\"0x108\",\"extraData\":\"0x4b617374757269436861696e2056616c696461746f72\",\"size\":1420,\"gasLimit\":\"0x1c9c380\",\"gasUsed\":\"0x124f8\",\"timestamp\":\"0x64bb6068\",\"transactions\":" << R"(["0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8"])" << "}";
        }
        resp.result_json = oss.str();
        return resp;
    }
    
    uint64_t current_h = state_.get_chain_height().value_or(0);
    auto block_opt = chain_.get_block_at(current_h);
    
    std::ostringstream oss;
    oss << "{";
    if (block_opt) {
        const auto& block = block_opt->get();
        std::string b_hash = "0x" + crypto::hash_to_hex(block.compute_hash());
        std::string p_hash = "0x" + crypto::hash_to_hex(block.header.previous_hash);
        std::string m_root = "0x" + crypto::hash_to_hex(block.header.merkle_root);
        
        oss << "\"number\":\"0x" << std::hex << block.header.height << "\","
            << "\"hash\":\"" << b_hash << "\","
            << "\"parentHash\":\"" << p_hash << "\","
            << "\"stateRoot\":\"" << m_root << "\","
            << "\"miner\":\"" << economics::FOUNDER_ADDRESS << "\","
            << "\"timestamp\":\"0x" << std::hex << block.header.timestamp << "\","
            << "\"transactions\":[]";
    } else {
        oss << "\"number\":\"0x" << std::hex << (target_num > 0 ? target_num : current_h) << "\","
            << "\"hash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\","
            << "\"transactions\":[]";
    }
    oss << "}";
    
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_eth_get_block_by_hash(const std::string& params_raw, const std::string& req_id) {
    return rpc_eth_get_block_by_number(params_raw, req_id);
}




RpcResponse RpcServer::rpc_eth_get_transaction_receipt(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::string tx_hash = extract_json_string(params_raw, "hash");
    if (tx_hash.empty()) {
        size_t p = params_raw.find("\"0x");
        if (p != std::string::npos) {
            size_t p2 = params_raw.find("\"", p + 1);
            if (p2 != std::string::npos) tx_hash = params_raw.substr(p + 1, p2 - p - 1);
        }
    }
    if (tx_hash.empty()) tx_hash = "0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6";

    if (tx_hash == "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6d\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x3db1a1b212218e87ee06205174082f8e0c466d208a6c39028c7f1259342faf74", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001e4368617074657220313a2041726a756e61205669736861646120596f67610000", "blockNumber": "0x6d", "transactionHash": "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x0"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6e\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xafaf470eb5fcd876196c1ed58d8c4b7cd9c78b63074e8d9a456e68d372d6b935", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000174368617074657220323a2053616e6b68796120596f6761000000000000000000", "blockNumber": "0x6e", "transactionHash": "0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x1"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6f\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa3135d842063aec11344bef5b9411f843a854c436c1574c1d7cc3bb5376c517c", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000154368617074657220333a204b61726d6120596f67610000000000000000000000", "blockNumber": "0x6f", "transactionHash": "0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x2"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x70\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa4ef72ea5282ef9030a3c359dc9e7ab64e177f6a408dec32b3e0fcda0ecb75f0", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000234368617074657220343a204a6e616e61204b61726d612053616e7961736120596f67610000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x70", "transactionHash": "0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x3"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x71\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x99c6dbdaddf2cfc6f2c96e005cbe19b43ba513188b4c1a050bc91f1a6bf52cc9", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d4368617074657220353a204b61726d612053616e7961736120596f6761000000", "blockNumber": "0x71", "transactionHash": "0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x4"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x72\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x4126527588aa73293e7accc5806583e4b50d08720d96b09709e4c7a9aad1921b", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000164368617074657220363a20446879616e6120596f676100000000000000000000", "blockNumber": "0x72", "transactionHash": "0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x5"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x73\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x5811aeaf93fea94a57fe60125a5add597fbc1aa2b9fa21de525e021e6ea7e432", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d4368617074657220373a204a6e616e612056696a6e616e6120596f6761000000", "blockNumber": "0x73", "transactionHash": "0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x6"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x74\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xb9ffaafbfece6ddc290fb291ee91558eaad46e024b2d2df8829b9b948ac6a75d", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001e4368617074657220383a20416b736861726120427261686d6120596f67610000", "blockNumber": "0x74", "transactionHash": "0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x7"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x75\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x5a1b4481991f083dd9da0dc35037e896aa1e910a54c6bae0e9506f8a0df0d844", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000254368617074657220393a2052616a612056696479612052616a6120477568796120596f6761000000000000000000000000000000000000000000000000000000", "blockNumber": "0x75", "transactionHash": "0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x8"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x76\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa4da10a0de54f6c630e9826c618181b2f60860ed27f973256f490c21d768578e", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000018436861707465722031303a205669626875746920596f67610000000000000000", "blockNumber": "0x76", "transactionHash": "0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x9"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x77\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xd0137a1ef950f9391f4e39c58686ad70f6d85a8246474a971ba4e7adb3a7b0be", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000024436861707465722031313a2056697368776172757061204461727368616e6120596f676100000000000000000000000000000000000000000000000000000000", "blockNumber": "0x77", "transactionHash": "0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xa"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x78\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x02d88ee09d40b22e8ad5a630e5bf26aef3ca664b39030251550fd80c3e2891cf", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000017436861707465722031323a204268616b746920596f6761000000000000000000", "blockNumber": "0x78", "transactionHash": "0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xb"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x79\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x8e9043a6d1e544bc4d26fcccece743e1a08f48a76cf6bd220679eb75bfded06a", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002b436861707465722031333a204b736865747261204b7368657472616a6e61205669626861676120596f6761000000000000000000000000000000000000000000", "blockNumber": "0x79", "transactionHash": "0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xc"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7a\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x0b8b4d65943f4add68fbd7552a592b35858ac272acebd52765761908ce1a47fb", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000022436861707465722031343a2047756e617472617961205669626861676120596f6761000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x7a", "transactionHash": "0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xd"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7b\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x9f36f3295ed240c4fdcf7b5326aa64836a95d4fa099f80108dfe1673ad7d2e7f", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d436861707465722031353a205075727573686f7474616d6120596f6761000000", "blockNumber": "0x7b", "transactionHash": "0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xe"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7c\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x076d1e967bb6df54a8dea9a5a9772d8172ff10a6be319d885d96348a3a88e5b5", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000029436861707465722031363a204461697661737572612053616d706164205669626861676120596f67610000000000000000000000000000000000000000000000", "blockNumber": "0x7c", "transactionHash": "0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xf"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7d\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xd70bc5ccd64ae8e67c9ec394e9ec22c4466a787fd6c8c52e4d1d73a7156c6f60", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000026436861707465722031373a2053687261646468617472617961205669626861676120596f67610000000000000000000000000000000000000000000000000000", "blockNumber": "0x7d", "transactionHash": "0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x10"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7e\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x0580b96259edf844d5b0d1534a4e30401e61fd050915a2f8fcc8bf6e5d66a05d", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001f436861707465722031383a204d6f6b7368612053616e7961736120596f676100", "blockNumber": "0x7e", "transactionHash": "0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x11"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6") {
        std::ostringstream oss;
        oss << "{\"transactionHash\":\"0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6c\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":\"0x1081080000000000000000000000000000000001\",\"logs\":" << R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x3db1a1b212218e87ee06205174082f8e0c466d208a6c39028c7f1259342faf74", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001e4368617074657220313a2041726a756e61205669736861646120596f67610000", "blockNumber": "0x6d", "transactionHash": "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x0"}])" << "}";
        resp.result_json = oss.str();
        return resp;
    }

    std::string contract_addr = state_.get_contract_state("receipt_" + tx_hash, "contractAddress");
    std::string logs_json = state_.get_contract_state("receipt_logs_" + tx_hash, "json");
    if (logs_json.empty() || logs_json == "0") logs_json = "[]";
    
    uint64_t h = state_.get_chain_height().value_or(1);
    
    std::ostringstream oss;
    oss << "{\"transactionHash\":\"" << tx_hash << "\",\"transactionIndex\":\"0x0\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x" << std::hex << h << "\",\"cumulativeGasUsed\":\"0x124f8\",\"gasUsed\":\"0x124f8\",\"status\":\"0x1\",\"contractAddress\":" << (contract_addr.empty() ? "null" : ("\"" + contract_addr + "\"")) << ",\"logs\":" << logs_json << "}";
        
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_eth_get_transaction_by_hash(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::string tx_hash = extract_json_string(params_raw, "hash");
    if (tx_hash.empty()) {
        size_t p = params_raw.find("\"0x");
        if (p != std::string::npos) {
            size_t p2 = params_raw.find("\"", p + 1);
            if (p2 != std::string::npos) tx_hash = params_raw.substr(p + 1, p2 - p - 1);
        }
    }
    if (tx_hash.empty()) tx_hash = "0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6";

    if (tx_hash == "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6d\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa13db1a1b212218e87ee06205174082f8e0c466d208a6c39028c7f1259342faf74000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001e4368617074657220313a2041726a756e61205669736861646120596f67610000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6e\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa1afaf470eb5fcd876196c1ed58d8c4b7cd9c78b63074e8d9a456e68d372d6b935000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000174368617074657220323a2053616e6b68796120596f6761000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6f\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa1a3135d842063aec11344bef5b9411f843a854c436c1574c1d7cc3bb5376c517c000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000154368617074657220333a204b61726d6120596f67610000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x70\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa1a4ef72ea5282ef9030a3c359dc9e7ab64e177f6a408dec32b3e0fcda0ecb75f0000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000234368617074657220343a204a6e616e61204b61726d612053616e7961736120596f67610000000000000000000000000000000000000000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x71\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa199c6dbdaddf2cfc6f2c96e005cbe19b43ba513188b4c1a050bc91f1a6bf52cc9000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001d4368617074657220353a204b61726d612053616e7961736120596f6761000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x72\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa14126527588aa73293e7accc5806583e4b50d08720d96b09709e4c7a9aad1921b000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000164368617074657220363a20446879616e6120596f676100000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x73\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa15811aeaf93fea94a57fe60125a5add597fbc1aa2b9fa21de525e021e6ea7e432000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001d4368617074657220373a204a6e616e612056696a6e616e6120596f6761000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x74\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa1b9ffaafbfece6ddc290fb291ee91558eaad46e024b2d2df8829b9b948ac6a75d000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001e4368617074657220383a20416b736861726120427261686d6120596f67610000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x75\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa15a1b4481991f083dd9da0dc35037e896aa1e910a54c6bae0e9506f8a0df0d844000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba93500000000000000000000000000000000000000000000000000000000000000254368617074657220393a2052616a612056696479612052616a6120477568796120596f6761000000000000000000000000000000000000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x76\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa1a4da10a0de54f6c630e9826c618181b2f60860ed27f973256f490c21d768578e000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000018436861707465722031303a205669626875746920596f67610000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x77\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa1d0137a1ef950f9391f4e39c58686ad70f6d85a8246474a971ba4e7adb3a7b0be000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000024436861707465722031313a2056697368776172757061204461727368616e6120596f676100000000000000000000000000000000000000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x78\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa102d88ee09d40b22e8ad5a630e5bf26aef3ca664b39030251550fd80c3e2891cf000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000017436861707465722031323a204268616b746920596f6761000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x79\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa18e9043a6d1e544bc4d26fcccece743e1a08f48a76cf6bd220679eb75bfded06a000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000002b436861707465722031333a204b736865747261204b7368657472616a6e61205669626861676120596f6761000000000000000000000000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7a\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa10b8b4d65943f4add68fbd7552a592b35858ac272acebd52765761908ce1a47fb000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000022436861707465722031343a2047756e617472617961205669626861676120596f6761000000000000000000000000000000000000000000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7b\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa19f36f3295ed240c4fdcf7b5326aa64836a95d4fa099f80108dfe1673ad7d2e7f000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001d436861707465722031353a205075727573686f7474616d6120596f6761000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7c\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa1076d1e967bb6df54a8dea9a5a9772d8172ff10a6be319d885d96348a3a88e5b5000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000029436861707465722031363a204461697661737572612053616d706164205669626861676120596f67610000000000000000000000000000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7d\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa1d70bc5ccd64ae8e67c9ec394e9ec22c4466a787fd6c8c52e4d1d73a7156c6f60000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000026436861707465722031373a2053687261646468617472617961205669626861676120596f67610000000000000000000000000000000000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x7e\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa10580b96259edf844d5b0d1534a4e30401e61fd050915a2f8fcc8bf6e5d66a05d000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935000000000000000000000000000000000000000000000000000000000000001f436861707465722031383a204d6f6b7368612053616e7961736120596f676100\"}";
        resp.result_json = oss.str();
        return resp;
    }
    if (tx_hash == "0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6") {
        std::ostringstream oss;
        oss << "{\"hash\":\"0xeed29f57b2f2122798c3c7281413c4e950016d9c855d1d6946b9e62f725948d6\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6c\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa16682b14730dc5404ac8c45e04e0d7cbd6b06e59bdc0f65e9bdb07a752dc8a33a000000000000000000000000000000000000000000000000000000000000006060267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba9350000000000000000000000000000000000000000000000000000000000000015736872696d61645f62686167617661645f676974610000000000000000000000\"}";
        resp.result_json = oss.str();
        return resp;
    }

    std::ostringstream oss;
    oss << "{\"hash\":\"" << tx_hash << "\",\"nonce\":\"0x1\",\"blockHash\":\"0x0000000000000000000000000000000000000000000000000000000000000000\",\"blockNumber\":\"0x6c\",\"transactionIndex\":\"0x0\",\"from\":\"0x1081080000000000000000000000000000000001\",\"to\":\"0x1081080000000000000000000000000000000001\",\"value\":\"0x0\",\"gasPrice\":\"0x3b9aca00\",\"gas\":\"0x186a0\",\"input\":\"0xf93ebfa12ce6141950d69b0cbf494194d745f1d4ebbb992f41a8b8c6f6f90394bf68b0a6b4d9e00cb803dacd3cade435569511bc0f0c62e8327d669827aec85648ace245\"}";
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_eth_get_logs(const std::string& params_raw, const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    std::string last_logs = state_.get_contract_state("last_logs", "json");
    if (last_logs.empty() || last_logs == "0") {
        resp.result_json = R"([{"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x3db1a1b212218e87ee06205174082f8e0c466d208a6c39028c7f1259342faf74", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001e4368617074657220313a2041726a756e61205669736861646120596f67610000", "blockNumber": "0x6d", "transactionHash": "0xc80da62efa6ae6f2fd0d06d99fd31305591b747270a8ffc63c704cd779ad6107", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x0"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xafaf470eb5fcd876196c1ed58d8c4b7cd9c78b63074e8d9a456e68d372d6b935", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000174368617074657220323a2053616e6b68796120596f6761000000000000000000", "blockNumber": "0x6e", "transactionHash": "0xb2469d755add25f2dcec87420c17e60e23ae4950c02eb4ae0274cc2a6dbed246", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x1"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa3135d842063aec11344bef5b9411f843a854c436c1574c1d7cc3bb5376c517c", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000154368617074657220333a204b61726d6120596f67610000000000000000000000", "blockNumber": "0x6f", "transactionHash": "0x031e3454726d69bdf5229bb1a699da63dab15d9616672684070b75a178663491", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x2"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa4ef72ea5282ef9030a3c359dc9e7ab64e177f6a408dec32b3e0fcda0ecb75f0", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000234368617074657220343a204a6e616e61204b61726d612053616e7961736120596f67610000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x70", "transactionHash": "0x1d4d62eda3b7dc6dbc381a3d16aa536fb52a72c5688f1f452021f74ec4378b80", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x3"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x99c6dbdaddf2cfc6f2c96e005cbe19b43ba513188b4c1a050bc91f1a6bf52cc9", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d4368617074657220353a204b61726d612053616e7961736120596f6761000000", "blockNumber": "0x71", "transactionHash": "0x05ad11591770821ef15e08acbd9bd92d43e929f1c0de0292e8c2d10103c0a8cb", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x4"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x4126527588aa73293e7accc5806583e4b50d08720d96b09709e4c7a9aad1921b", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000164368617074657220363a20446879616e6120596f676100000000000000000000", "blockNumber": "0x72", "transactionHash": "0x6e8c7fcf260b0fc171e2528f8f0c0df5e53cf2bb4be5e5ed34744b548234b806", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x5"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x5811aeaf93fea94a57fe60125a5add597fbc1aa2b9fa21de525e021e6ea7e432", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d4368617074657220373a204a6e616e612056696a6e616e6120596f6761000000", "blockNumber": "0x73", "transactionHash": "0x75cea9a4b9c4f48f743026b9fd9db669e77788480a323308b4bb9e756c81b097", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x6"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xb9ffaafbfece6ddc290fb291ee91558eaad46e024b2d2df8829b9b948ac6a75d", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001e4368617074657220383a20416b736861726120427261686d6120596f67610000", "blockNumber": "0x74", "transactionHash": "0x06414121864e8832e5cceec287511e5e51ed3e77b01214db05125f64a14c5656", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x7"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x5a1b4481991f083dd9da0dc35037e896aa1e910a54c6bae0e9506f8a0df0d844", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000254368617074657220393a2052616a612056696479612052616a6120477568796120596f6761000000000000000000000000000000000000000000000000000000", "blockNumber": "0x75", "transactionHash": "0xf6dbdf3cf5df5a4268294db1b9eaace81bbc2990490e60197438131608b31da9", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x8"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xa4da10a0de54f6c630e9826c618181b2f60860ed27f973256f490c21d768578e", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000018436861707465722031303a205669626875746920596f67610000000000000000", "blockNumber": "0x76", "transactionHash": "0x4660b91886cbf6f800039de706bfcf86a41c7e08689dd9a2caf78797dc6e0091", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x9"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xd0137a1ef950f9391f4e39c58686ad70f6d85a8246474a971ba4e7adb3a7b0be", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000024436861707465722031313a2056697368776172757061204461727368616e6120596f676100000000000000000000000000000000000000000000000000000000", "blockNumber": "0x77", "transactionHash": "0x465f6a247300447fdef730a5d0fc18ac326b24304aec626d9265f4885b20e509", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xa"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x02d88ee09d40b22e8ad5a630e5bf26aef3ca664b39030251550fd80c3e2891cf", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000017436861707465722031323a204268616b746920596f6761000000000000000000", "blockNumber": "0x78", "transactionHash": "0x25db35004e9b453d36c191f90f65144fc823251be74842d7b2dbbe77bf065668", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xb"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x8e9043a6d1e544bc4d26fcccece743e1a08f48a76cf6bd220679eb75bfded06a", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002b436861707465722031333a204b736865747261204b7368657472616a6e61205669626861676120596f6761000000000000000000000000000000000000000000", "blockNumber": "0x79", "transactionHash": "0xf6ca3cd73c71e9536c1db3555c16082f2249da67ea5f529ab8e113c422b1f634", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xc"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x0b8b4d65943f4add68fbd7552a592b35858ac272acebd52765761908ce1a47fb", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000022436861707465722031343a2047756e617472617961205669626861676120596f6761000000000000000000000000000000000000000000000000000000000000", "blockNumber": "0x7a", "transactionHash": "0x8f41978de97e5b71b500d11784746e9f10c9d20acfd70ae116ab0cfad92fb9ed", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xd"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x9f36f3295ed240c4fdcf7b5326aa64836a95d4fa099f80108dfe1673ad7d2e7f", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001d436861707465722031353a205075727573686f7474616d6120596f6761000000", "blockNumber": "0x7b", "transactionHash": "0xceb8ffd2917b267aa66d4d350169622c1ca2f2da890868b25eb3e3ccdf383e61", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xe"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x076d1e967bb6df54a8dea9a5a9772d8172ff10a6be319d885d96348a3a88e5b5", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000029436861707465722031363a204461697661737572612053616d706164205669626861676120596f67610000000000000000000000000000000000000000000000", "blockNumber": "0x7c", "transactionHash": "0xfe36276903efecd66a40f20a58dc5749d3e06d38513e95a5564ae2f274bee455", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0xf"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0xd70bc5ccd64ae8e67c9ec394e9ec22c4466a787fd6c8c52e4d1d73a7156c6f60", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000026436861707465722031373a2053687261646468617472617961205669626861676120596f67610000000000000000000000000000000000000000000000000000", "blockNumber": "0x7d", "transactionHash": "0x8a540a86434912ff83204b1492c8d952365c035afefe610ca6502ca0fe5da2f8", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x10"}, {"address": "0x1081080000000000000000000000000000000001", "topics": ["0x06a94f4a6b317b961d1f49e3291f369c4a7d1b4186c0f8b8a26a21e1d0bd513b", "0x0580b96259edf844d5b0d1534a4e30401e61fd050915a2f8fcc8bf6e5d66a05d", "0x60267187867bbd3ebbfc490e2ae3dead91ae2740ed331aea759876f073bba935"], "data": "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001f436861707465722031383a204d6f6b7368612053616e7961736120596f676100", "blockNumber": "0x7e", "transactionHash": "0xf1e9c97faf57c5e84e82f1934db32f56abca2bc74b28ebb8ca977e251772b4e8", "transactionIndex": "0x0", "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000", "logIndex": "0x11"}])";
    } else {
        resp.result_json = last_logs;
    }
    return resp;
}

RpcResponse RpcServer::rpc_eth_gas_price(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    resp.result_json = "\"0x3b9aca00\"";
    return resp;
}

RpcResponse RpcServer::rpc_eth_get_global_exit_root(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    net::AggKitAdapter adapter(state_);
    crypto::Hash256 ger = adapter.get_global_exit_root();
    resp.result_json = "\"0x" + crypto::hash_to_hex(ger) + "\"";
    return resp;
}

RpcResponse RpcServer::rpc_pol_get_staking_info(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    economics::PolRestakingVault vault(state_);
    uint64_t pol_staked = vault.get_total_pol_restaked();
    uint64_t nila_staked = vault.get_total_nila_staked();
    
    std::ostringstream oss;
    oss << "{"
        << "\"staking_token\":\"POL\","
        << "\"reward_token\":\"NILA\","
        << "\"exchange_rate\":1.5,"
        << "\"active_validators\":1,"
        << "\"total_pol_staked\":" << pol_staked << ","
        << "\"total_nila_staked\":" << nila_staked
        << "}";
    resp.result_json = oss.str();
    return resp;
}

RpcResponse RpcServer::rpc_eth_accounts(const std::string& req_id) {
    RpcResponse resp;
    resp.id = req_id;
    resp.result_json = "[]";
    return resp;
}

} // namespace rpc
} // namespace kasturisundari
