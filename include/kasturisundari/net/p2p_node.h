// Kasturisundari Chain — P2P Node (TCP Server & Client)
// Zero-dependency POSIX sockets implementation for maximum OPSEC.
// Handles network communication, peer discovery, and message broadcasting.

#pragma once

#include "kasturisundari/net/p2p_message.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

namespace kasturisundari {
namespace net {

// Forward declaration for the internal peer structure
struct PeerConnection;

/// A node on the Kasturisundari P2P network.
class P2PNode {
public:
    /// Callback signature for handling incoming messages.
    /// Parameters: peer_id, message.
    using MessageHandler = std::function<void(uint64_t, const P2PMessage&)>;

    P2PNode(uint16_t port);
    ~P2PNode();

    /// Start listening for incoming connections and start network threads.
    bool start();

    /// Stop all networking and close all connections.
    void stop();

    /// Connect to a remote peer (IP and Port).
    bool connect_to_peer(const std::string& ip_address, uint16_t port);

    /// Broadcast a message to all connected peers.
    void broadcast(const P2PMessage& msg);

    /// Send a message to a specific peer.
    void send_to_peer(uint64_t peer_id, const P2PMessage& msg);

    /// Set the callback for when a valid P2PMessage is received.
    void set_message_handler(MessageHandler handler);

    /// Get the number of currently connected peers.
    size_t connected_peers_count() const;

private:
    uint16_t port_;
    std::atomic<bool> running_{false};
    int server_socket_{-1};

    std::thread listener_thread_;
    std::thread worker_thread_;

    mutable std::mutex peers_mtx_;
    uint64_t next_peer_id_{1};
    std::unordered_map<uint64_t, std::shared_ptr<PeerConnection>> peers_;

    MessageHandler msg_handler_;

    void listener_loop();
    void worker_loop();
    
    void handle_new_connection(int client_sock, const std::string& ip, uint16_t port);
    void disconnect_peer(uint64_t peer_id);
};

} // namespace net
} // namespace kasturisundari
