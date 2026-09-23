// Kasturisundari Chain — Daemon (kasturid)
// The main entry point for running a full sovereign node.

#include "kasturisundari/state/state_db.h"
#include "kasturisundari/core/blockchain.h"
#include "kasturisundari/mempool/mempool.h"
#include "kasturisundari/net/p2p_node.h"
#include "kasturisundari/rpc/rpc_server.h"
#include "kasturisundari/poc/poc_forger.h"
#include "kasturisundari/economics/tokenomics.h"
#include "kasturisundari/governance/sabha.h"
#include "kasturisundari/storage/vedic_storage.h"
#include "kasturisundari/genesis/genesis.h"
#include "kasturisundari/vm/agni_machine.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

using namespace kasturisundari;

std::atomic<bool> keep_running{true};

void signal_handler(int) {
    std::cout << "\n[kasturid] Shutdown signal received. Gracefully stopping..." << std::endl;
    keep_running = false;
}

int main(int argc, char** argv) {
    std::cout << "==================================================" << std::endl;
    std::cout << "  Kasturisundari Chain Sovereign Node (kasturid)  " << std::endl;
    std::cout << "  Version 1.0.0-rc1 | Zero-Dependency Mode        " << std::endl;
    std::cout << "==================================================" << std::endl;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Parse CLI arguments
    std::string connect_peer = "";
    std::string miner_address = economics::FOUNDER_ADDRESS; // Default to founder
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--connect" && i + 1 < argc) {
            connect_peer = argv[++i];
        } else if (arg == "--miner" && i + 1 < argc) {
            miner_address = argv[++i];
        }
    }

    // 1. Initialize StateDB
    std::cout << "[kasturid] Initializing StateDB..." << std::endl;
    state::StateDB state;
    if (!state.open("./kasturi_data")) {
        std::cerr << "[kasturid] FATAL: Failed to open state database!" << std::endl;
        return 1;
    }

    // 2. Initialize Core Subsystems
    mempool::Mempool mempool;
    core::Blockchain blockchain; // Uses default constructor
    
    // Check if genesis needs to be initialized
    if (!state.get_latest_block_hash().has_value()) {
        std::cout << "[kasturid] Database empty. Initializing Genesis Block..." << std::endl;
        genesis::GenesisConfig cfg;
        cfg.founder_address = economics::FOUNDER_ADDRESS;
        cfg.secondary_address = economics::SECONDARY_ADDRESS;
        cfg.reward_pool_address = economics::REWARD_POOL_ADDRESS;
        cfg.timestamp = 1690000000;
        
        core::Block gen_block = genesis::build_genesis_block(cfg);
        genesis::apply_genesis_to_state(gen_block, state);
        blockchain.initialize_genesis(gen_block);
        std::cout << "[kasturid] Genesis block applied. Founder balance: 1,000,000 NILA" << std::endl;
    }

    
    // 2.1 Initialize Vedic Storage
    std::cout << "[kasturid] Initializing Vedic Decentralized Storage..." << std::endl;
    storage::VedicStorage vstorage("./kasturi_storage");
    if (!vstorage.init()) {
        std::cerr << "[kasturid] FATAL: Failed to init VedicStorage!" << std::endl;
        return 1;
    }

    // 3. Initialize Networking (P2P)
    uint16_t p2p_port = 18501;
    net::P2PNode p2p_node(p2p_port);
    
    // Wire P2P messages to Blockchain/Mempool
    p2p_node.set_message_handler([&](uint64_t peer_id, const net::P2PMessage& msg) {
        if (msg.command == "ping") {
            p2p_node.send_to_peer(peer_id, net::P2PMessage("pong", {}));
        } else if (msg.command == "tx") {
            try {
                core::Transaction tx = core::Transaction::deserialize(msg.payload);
                if (tx.verify_signature()) {
                    auto res = mempool.add_transaction(tx, state);
                    if (res == mempool::TxRejectReason::NONE) {
                        std::cout << "[P2P] Received and accepted new tx: " << tx.txid() << std::endl;
                    }
                }
            } catch (...) {}
        } else if (msg.command == "block") {
            try {
                core::Block block = core::Block::deserialize(msg.payload);
                auto res = blockchain.add_block(block);
                if (res == core::AddBlockResult::SUCCESS) {
                    state.put_latest_block_hash(block.compute_hash());
                    state.put_chain_height(block.header.height);
                    std::cout << "[P2P] Accepted new block #" << block.header.height << " from network\n";
                }
            } catch (...) {}
        }
    });

    if (p2p_node.start()) {
        std::cout << "[kasturid] P2P Node listening on TCP port " << p2p_port << std::endl;
        
        // Connect to external peer if specified, otherwise connect to seed node
        if (connect_peer.empty()) {
            connect_peer = "52.72.236.75:18501";
        }
        
        if (!connect_peer.empty()) {
            size_t colon_pos = connect_peer.find(':');
            if (colon_pos != std::string::npos) {
                std::string ip = connect_peer.substr(0, colon_pos);
                uint16_t port = std::stoi(connect_peer.substr(colon_pos + 1));
                std::cout << "[kasturid] Attempting to connect to peer " << ip << ":" << port << "..." << std::endl;
                if (p2p_node.connect_to_peer(ip, port)) {
                    std::cout << "[kasturid] Successfully connected to " << ip << ":" << port << std::endl;
                } else {
                    std::cerr << "[kasturid] Failed to connect to " << ip << ":" << port << std::endl;
                }
            }
        }
    } else {
        std::cerr << "[kasturid] FATAL: Failed to start P2P node!" << std::endl;
        return 1;
    }

    // 4. Initialize Web3 JSON-RPC
    uint16_t rpc_port = 8545;
    rpc::RpcServer rpc_server(rpc_port, state, mempool, blockchain, vstorage);
    
    // Broadcast RPC txs to P2P network
    rpc_server.set_tx_broadcast_callback([&p2p_node](const core::Transaction& tx) {
        // Serialize and broadcast to the P2P network
        net::P2PMessage msg("tx", tx.serialize());
        p2p_node.broadcast(msg);
        std::cout << "[P2P] Gossiping transaction: " << tx.txid() << std::endl;
    });

    if (rpc_server.start()) {
        std::cout << "[kasturid] JSON-RPC Server listening on HTTP port " << rpc_port << std::endl;
    } else {
        std::cerr << "[kasturid] FATAL: Failed to start JSON-RPC server!" << std::endl;
        return 1;
    }

    std::cout << "\n[kasturid] Node is fully operational and syncing." << std::endl;
    std::cout << "[kasturid] Press Ctrl+C to exit safely." << std::endl;

    // Main event loop (The PoC Forger)
    int tick = 0;
    // Get our node's PoC score to determine forging interval
    std::string my_address = miner_address; // Node's own address for mining rewards
    uint64_t my_poc_score = poc::get_poc_score(my_address, state);
    uint64_t forging_interval = poc::compute_forging_interval(my_poc_score);
    
    std::cout << "[kasturid] PoC Score: " << my_poc_score 
              << " | Forging interval: " << forging_interval << "s" << std::endl;
    
    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        tick++;
        
        // Forge a block based on PoC-weighted interval
        if (tick >= (int)forging_interval) {
            tick = 0;
            
            // Refresh PoC score periodically (may change if contributions are approved)
            my_poc_score = poc::get_poc_score(my_address, state);
            forging_interval = poc::compute_forging_interval(my_poc_score);
            
            // Gather transactions
            auto txs = mempool.get_pending_transactions(500);
            if (!txs.empty()) {
                std::cout << "[Forger] Mempool has " << txs.size() << " pending transactions to include." << std::endl;
                for (const auto& t : txs) {
                    std::cout << "[Forger]   TX: " << t.txid() << " type=" << (int)t.type << " sender=" << t.sender.substr(0,16) << "..." << std::endl;
                }
            }
            
            // Create block
            core::Block block;
            block.header.version = 1;
            block.header.height = blockchain.get_height() + 1;
            block.header.previous_hash = blockchain.get_tip_hash();
            block.header.timestamp = std::time(nullptr);
            block.transactions = txs;
            
            // Tally fees
            uint64_t total_fees = 0;
            for (const auto& tx : txs) total_fees += tx.fee;
            block.header.total_fees = total_fees;
            block.header.tx_count = txs.size();
            
            // Compute merkle root
            block.header.merkle_root = block.compute_merkle_root();
            
            // Validate and Add
            core::AddBlockResult res = blockchain.add_block(block);
            if (res != core::AddBlockResult::SUCCESS) {
                std::cerr << "[Forger] FAILED to add block #" << block.header.height 
                          << " reason: " << core::add_block_result_to_string(res) << std::endl;
            }
            if (res == core::AddBlockResult::SUCCESS) {
                // Remove mined transactions from mempool
                mempool.remove_transactions(txs);
                
                // === EXECUTE STATE TRANSITIONS ===
                for (const auto& tx : txs) {
                    // Log History
                    state.add_transaction_history(tx.sender, tx.txid());
                    if (tx.sender != tx.recipient && !tx.recipient.empty()) {
                        state.add_transaction_history(tx.recipient, tx.txid());
                    }

                    // Always deduct fee and increment nonce for sender
                    state::Account sender_acc = state.get_account(tx.sender);
                    sender_acc.balance -= tx.fee;
                    sender_acc.nonce = tx.nonce;
                    state.put_account(tx.sender, sender_acc);
                    
                    if (tx.type == core::TxType::TRANSFER) { // TRANSFER
                        if (sender_acc.balance >= tx.amount) {
                            sender_acc = state.get_account(tx.sender);
                            sender_acc.balance -= tx.amount;
                            state.put_account(tx.sender, sender_acc);
                            
                            state::Account rec_acc = state.get_account(tx.recipient);
                            rec_acc.balance += tx.amount;
                            state.put_account(tx.recipient, rec_acc);
                        }
                    } else if (tx.type == core::TxType::CONTRACT_DEPLOY) { // CONTRACT_DEPLOY
                        // Use tx.txid() as the contract address
                        std::string contract_address = "contract_" + tx.txid();
                        std::string bytecode_hex;
                        for (uint8_t b : tx.data) {
                            char buf[3];
                            snprintf(buf, sizeof(buf), "%02x", b);
                            bytecode_hex += buf;
                        }
                        state.set_contract_state(contract_address, "bytecode", bytecode_hex);
                        std::cout << "[Agni VM] Deployed Contract: " << contract_address << std::endl;
                    } else if (tx.type == core::TxType::CONTRACT_CALL) { // CONTRACT_CALL
                        std::string contract_address = tx.recipient; // The contract to call
                        std::string bytecode_hex = state.get_contract_state(contract_address, "bytecode");
                        if (bytecode_hex != "0" && !bytecode_hex.empty()) {
                            // Dummy AgniVM execution for now (Full bytecode parsing requires VM integration)
                            vm::AgniMachine machine(10000); // 10k Prana
                            vm::ContractState cstate;
                            machine.execute(cstate, &state);
                            std::cout << "[Agni VM] Executed call to " << contract_address << std::endl;
                        }
                    }
                }
                
                // === MINING BLOCK REWARD ===
                uint64_t block_reward = 50 * economics::UNITS_PER_NILA; // 50 NILA Reward
                state::Account pool_acc = state.get_account(economics::REWARD_POOL_ADDRESS);
                if (pool_acc.balance >= block_reward) {
                    pool_acc.balance -= block_reward;
                    state.put_account(economics::REWARD_POOL_ADDRESS, pool_acc);
                    
                    state::Account miner_acc = state.get_account(my_address);
                    miner_acc.balance += block_reward;
                    state.put_account(my_address, miner_acc);
                } else {
                    block_reward = 0; // Pool is empty!
                }
                
                // === FEE RECYCLING & MINER DISTRIBUTION ===
                // Give all collected fees to the miner
                if (total_fees > 0) {
                    state::Account miner_acc = state.get_account(my_address);
                    miner_acc.balance += total_fees;
                    state.put_account(my_address, miner_acc);
                    std::cout << "[Reward] Miner " << my_address.substr(0, 10) << "... earned " 
                              << economics::format_nila(block_reward + total_fees) << " NILA" << std::endl;
                }
                
                // Update StateDB metadata
                state.put_latest_block_hash(block.compute_hash());
                state.put_chain_height(block.header.height);
                
                std::cout << "[Forger] Mined block #" << block.header.height 
                          << " with " << txs.size() << " transactions. Hash: " 
                          << crypto::hash_to_hex(block.compute_hash()).substr(0, 16) << "..." << std::endl;
                          
                // Broadcast block to P2P network
                p2p_node.broadcast(net::P2PMessage("block", block.serialize()));
            }
        }
    }

    // Graceful Shutdown
    std::cout << "[kasturid] Shutting down JSON-RPC..." << std::endl;
    rpc_server.stop();
    
    std::cout << "[kasturid] Shutting down P2P network..." << std::endl;
    p2p_node.stop();
    
    std::cout << "[kasturid] Closing database safely..." << std::endl;
    state.close();

    std::cout << "[kasturid] Shutdown complete." << std::endl;
    return 0;
}
