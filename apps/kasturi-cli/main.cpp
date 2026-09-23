// Kasturisundari Chain — Command Line Interface (kasturi-cli)
// Used by administrators and developers to interact with the kasturid node.

#include "kasturisundari/crypto/address.h"
#include "kasturisundari/crypto/secp256k1_wrapper.h"
#include "kasturisundari/crypto/post_quantum.h"
#include "kasturisundari/attestation/text_attestation.h"
#include "kasturisundari/core/transaction.h"
#include "kasturisundari/economics/tokenomics.h"

#include <fstream>
#include <sstream>

#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace kasturisundari;

// Helper to make HTTP POST requests to the RPC Server
std::string rpc_call(const std::string& method, const std::string& params_json) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "Error: Failed to create socket";

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8545);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return "Error: Could not connect to kasturid on 127.0.0.1:8545. Is the daemon running?";
    }

    std::string json_body = "{\"jsonrpc\":\"2.0\",\"method\":\"" + method + "\"";
    if (!params_json.empty()) {
        json_body += ",\"params\":" + params_json;
    }
    json_body += ",\"id\":\"cli\"}";

    std::string http_req = 
        "POST / HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Length: " + std::to_string(json_body.length()) + "\r\n"
        "Content-Type: application/json\r\n\r\n" + json_body;

    size_t total_sent = 0;
    while (total_sent < http_req.length()) {
        ssize_t sent = send(sock, http_req.c_str() + total_sent, http_req.length() - total_sent, 0);
        if (sent <= 0) break;
        total_sent += sent;
    }

    char buffer[8192];
    ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    close(sock);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string resp(buffer);
        size_t header_end = resp.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            return resp.substr(header_end + 4);
        }
    }
    return "Error: Empty response";
}

void print_help() {
    std::cout << "Kasturisundari Chain CLI - Version 1.0.0\n";
    std::cout << "Usage: kasturi-cli <command> [args]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  info                         Get node status and blockchain height\n";
    std::cout << "  balance <address>            Check the NILA balance of an address\n";
    std::cout << "  generate_wallet              Generate a new offline K-address and keys\n";
    std::cout << "  submit_proposal <hash> <msg> Submit a Proof of Contribution proposal\n";
    std::cout << "  attest <file.kasturi>        Ingest and attest a sacred text to the blockchain\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string command = argv[1];

    if (command == "info") {
        std::cout << rpc_call("get_info", "") << "\n";
    } 
    else if (command == "balance") {
        if (argc < 3) {
            std::cout << "Error: Missing address.\nUsage: kasturi-cli balance <address>\n";
            return 1;
        }
        std::string addr = argv[2];
        std::string params = "{\"address\":\"" + addr + "\"}";
        std::cout << rpc_call("get_balance", params) << "\n";
    }
    else if (command == "generate_wallet") {
        std::cout << "Generating Sovereign Offline Wallet (OPSEC Mode)...\n";
        
        crypto::KeyPair class_keys = crypto::generate_keypair();
        crypto::pqc::PqPublicKey pq_pub;
        crypto::pqc::PqPrivateKey pq_priv;
        crypto::pqc::generate_pq_keypair(pq_pub, pq_priv);

        std::cout << "========================================================\n";
        std::cout << "Address (Public): " << class_keys.address << "\n";
        std::cout << "WARNING: Keep your private keys strictly confidential!\n";
        std::cout << "Classical Private Key: " << crypto::privkey_to_hex(class_keys.private_key) << "\n";
        std::cout << "Post-Quantum Private Key is stored securely (Mocked).\n";
        std::cout << "========================================================\n";
    }
    else if (command == "submit_proposal") {
        if (argc < 4) {
            std::cout << "Error: Missing arguments.\nUsage: kasturi-cli submit_proposal <commit_hash> <description>\n";
            return 1;
        }
        // In a full implementation, this would construct a Tx, sign it, and call send_transaction
        std::cout << "Proposal crafted offline. (Not implemented in demo).\n";
    }
    else if (command == "attest") {
        if (argc < 3) {
            std::cout << "Error: Missing file.\nUsage: kasturi-cli attest <file.kasturi>\n";
            return 1;
        }
        std::string filename = argv[2];
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: Could not open " << filename << "\n";
            return 1;
        }

        std::string title = "Unknown";
        uint8_t category = 255;
        std::string language = "Sanskrit";
        std::vector<attestation::Verse> verses;

        std::string line;
        while (std::getline(file, line)) {
            if (line.rfind("TITLE: ", 0) == 0) title = line.substr(7);
            else if (line.rfind("CATEGORY: ", 0) == 0) category = std::stoi(line.substr(10));
            else if (line.rfind("LANGUAGE: ", 0) == 0) language = line.substr(10);
            else if (line.rfind("VERSE: ", 0) == 0) {
                std::istringstream iss(line.substr(7));
                uint32_t ch, vnum;
                std::string content;
                if (iss >> ch >> vnum) {
                    std::getline(iss, content);
                    if (!content.empty() && content[0] == ' ') content = content.substr(1);
                    verses.push_back({ch, vnum, content});
                }
            }
        }

        std::cout << "Read " << verses.size() << " verses for: " << title << "\n";
        
        // 1. Compute Attestation
        auto att = attestation::attest_scripture(title, static_cast<attestation::TextCategory>(category), language, verses);
        auto att_bin = attestation::serialize_attestation(att);
        std::cout << "Merkle Root: " << crypto::hash_to_hex(att.merkle_root) << "\n";
        
        // 2. Send to Vedic Storage (RPC store_chunk)
        std::string hex_data;
        for (uint8_t b : att_bin) {
            char buf[3];
            sprintf(buf, "%02x", b);
            hex_data += buf;
        }
        std::string store_params = "{\"data_hex\":\"" + hex_data + "\"}";
        std::string store_resp = rpc_call("store_chunk", store_params);
        std::cout << "Storage RPC: " << store_resp << "\n";
        
        // 3. Create ATTESTATION Transaction
        // (Using founder's private key for demo to pay fees)
        crypto::PrivateKey founder_priv;
        std::string founder_hex = "f34458d601bce55523b0d2d3129598a3c4f74d081f9a2e374d7529431f6e2f1d"; // Dummy mock
        for (int i=0; i<32; i++) founder_priv[i] = std::stoul(founder_hex.substr(i*2, 2), nullptr, 16);
        
        auto pub_opt = crypto::derive_public_key(founder_priv);
        std::string actual_sender = crypto::public_key_to_address(pub_opt.value());

        // Fetch current nonce
        std::string nonce_params = "{\"address\":\"" + actual_sender + "\"}";
        std::string nonce_resp = rpc_call("get_nonce", nonce_params);
        uint64_t current_nonce = 0;
        
        // Parse nonce from {"jsonrpc":"2.0","result":"0","id":"cli"}
        size_t res_pos = nonce_resp.find("\"result\":\"");
        if (res_pos != std::string::npos) {
            size_t end_pos = nonce_resp.find("\"", res_pos + 10);
            if (end_pos != std::string::npos) {
                current_nonce = std::stoull(nonce_resp.substr(res_pos + 10, end_pos - (res_pos + 10)));
            }
        }
        
        core::Transaction tx;
        tx.version = 1;
        tx.type = core::TxType::ATTESTATION;
        tx.timestamp = time(nullptr);
        tx.nonce = current_nonce;
        tx.sender = actual_sender;
        tx.recipient = "";
        tx.amount = 0;
        tx.fee = 0; // 0 NILA
        tx.data = att_bin;
        
        if (tx.sign_transaction(founder_priv)) {
            auto tx_bin = tx.serialize();
            std::string tx_hex;
            for (uint8_t b : tx_bin) {
                char buf[3];
                sprintf(buf, "%02x", b);
                tx_hex += buf;
            }
            std::string tx_params = "{\"tx_hex\":\"" + tx_hex + "\"}";
            std::string tx_resp = rpc_call("send_transaction", tx_params);
            std::cout << "Tx RPC: " << tx_resp << "\n";
        }
    }
    else {
        std::cout << "Unknown command: " << command << "\n";
        print_help();
    }

    return 0;
}
