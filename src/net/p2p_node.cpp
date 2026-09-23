// Kasturisundari Chain — P2P Node Implementation (POSIX Sockets)
// Zero-dependency architecture.

#include "kasturisundari/net/p2p_node.h"

#include <iostream>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/select.h>

namespace kasturisundari {
namespace net {

struct PeerConnection {
    uint64_t id;
    int socket;
    std::string ip;
    uint16_t port;
    std::vector<uint8_t> recv_buffer;
    std::vector<uint8_t> send_buffer;
    std::mutex mtx; // Protects send_buffer

    PeerConnection(uint64_t pid, int sock, const std::string& ip_addr, uint16_t p)
        : id(pid), socket(sock), ip(ip_addr), port(p) {
        
        // Make socket non-blocking
        int flags = fcntl(socket, F_GETFL, 0);
        if (flags != -1) {
            fcntl(socket, F_SETFL, flags | O_NONBLOCK);
        }
    }

    ~PeerConnection() {
        if (socket != -1) {
            close(socket);
        }
    }
};

P2PNode::P2PNode(uint16_t port) : port_(port) {}

P2PNode::~P2PNode() {
    stop();
}

void P2PNode::set_message_handler(MessageHandler handler) {
    msg_handler_ = std::move(handler);
}

bool P2PNode::start() {
    if (running_) return false;

    // 1. Create server socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return false;
    }

    // Allow port reuse
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    // 2. Bind to any interface
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket_);
        return false;
    }

    // 3. Listen
    if (listen(server_socket_, 50) < 0) {
        close(server_socket_);
        return false;
    }

    // Make server socket non-blocking
    int flags = fcntl(server_socket_, F_GETFL, 0);
    fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);

    running_ = true;
    
    // 4. Start threads
    listener_thread_ = std::thread(&P2PNode::listener_loop, this);
    worker_thread_ = std::thread(&P2PNode::worker_loop, this);

    return true;
}

void P2PNode::stop() {
    if (!running_) return;
    
    running_ = false;
    
    if (listener_thread_.joinable()) listener_thread_.join();
    if (worker_thread_.joinable()) worker_thread_.join();

    if (server_socket_ != -1) {
        close(server_socket_);
        server_socket_ = -1;
    }

    std::lock_guard<std::mutex> lock(peers_mtx_);
    peers_.clear(); // Destructors will close the sockets
}

bool P2PNode::connect_to_peer(const std::string& ip_address, uint16_t port) {
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    std::string port_str = std::to_string(port);
    if (getaddrinfo(ip_address.c_str(), port_str.c_str(), &hints, &res) != 0) {
        return false;
    }
    
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return false;
    }

    // We do a blocking connect for simplicity, but timeout quickly in a real app.
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "[kasturid] connect() failed: " << strerror(errno) << std::endl;
        close(sock);
        freeaddrinfo(res);
        return false;
    }
    
    freeaddrinfo(res);

    handle_new_connection(sock, ip_address, port);
    return true;
}

void P2PNode::handle_new_connection(int client_sock, const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(peers_mtx_);
    uint64_t pid = next_peer_id_++;
    auto peer = std::make_shared<PeerConnection>(pid, client_sock, ip, port);
    peers_[pid] = peer;
}

void P2PNode::disconnect_peer(uint64_t peer_id) {
    std::lock_guard<std::mutex> lock(peers_mtx_);
    peers_.erase(peer_id); // Auto closes socket
}

void P2PNode::broadcast(const P2PMessage& msg) {
    auto data = msg.serialize();
    std::lock_guard<std::mutex> lock(peers_mtx_);
    for (auto& pair : peers_) {
        std::lock_guard<std::mutex> plock(pair.second->mtx);
        pair.second->send_buffer.insert(pair.second->send_buffer.end(), data.begin(), data.end());
    }
}

void P2PNode::send_to_peer(uint64_t peer_id, const P2PMessage& msg) {
    auto data = msg.serialize();
    std::lock_guard<std::mutex> lock(peers_mtx_);
    auto it = peers_.find(peer_id);
    if (it != peers_.end()) {
        std::lock_guard<std::mutex> plock(it->second->mtx);
        it->second->send_buffer.insert(it->second->send_buffer.end(), data.begin(), data.end());
    }
}

size_t P2PNode::connected_peers_count() const {
    std::lock_guard<std::mutex> lock(peers_mtx_);
    return peers_.size();
}

void P2PNode::listener_loop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock >= 0) {
            char ip_buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, INET_ADDRSTRLEN);
            handle_new_connection(client_sock, std::string(ip_buf), ntohs(client_addr.sin_port));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void P2PNode::worker_loop() {
    std::vector<uint8_t> rx_buf(8192);

    while (running_) {
        std::vector<uint64_t> to_disconnect;
        std::vector<std::pair<uint64_t, P2PMessage>> to_handle;
        
        {
            std::lock_guard<std::mutex> lock(peers_mtx_);
            for (auto& [id, peer] : peers_) {
                // 1. Receive data
                ssize_t bytes_rx = recv(peer->socket, rx_buf.data(), rx_buf.size(), 0);
                if (bytes_rx > 0) {
                    peer->recv_buffer.insert(peer->recv_buffer.end(), rx_buf.begin(), rx_buf.begin() + bytes_rx);
                    
                    // Parse all complete messages
                    P2PMessage msg;
                    while (P2PMessage::parse(peer->recv_buffer, msg)) {
                        // Initial Block Download (IBD) Handling
                        if (msg.command == "REQ_BLOCKS") {
                            P2PMessage res;
                            res.command = "RES_BLOCKS";
                            res.payload = {'B', 'L', 'O', 'C', 'K', 'S'};
                            send_to_peer(id, res);
                        } else if (msg.command == "RES_BLOCKS") {
                            std::cout << "[P2P] IBD: Received blocks from peer " << id << "\n";
                        } else if (msg.command == "REQ_CHUNK") {
                            P2PMessage res;
                            res.command = "RES_CHUNK";
                            res.payload = msg.payload;
                            send_to_peer(id, res);
                        } else if (msg.command == "RES_CHUNK") {
                            std::cout << "[P2P] Storage: Received chunk from peer " << id << "\n";
                        }
                        to_handle.push_back({id, msg});
                    }
                } else if (bytes_rx == 0 || (bytes_rx < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                    // Connection closed or error
                    to_disconnect.push_back(id);
                    continue;
                }

                // 2. Send data
                std::lock_guard<std::mutex> plock(peer->mtx);
                if (!peer->send_buffer.empty()) {
                    ssize_t bytes_tx = send(peer->socket, peer->send_buffer.data(), peer->send_buffer.size(), 0);
                    if (bytes_tx > 0) {
                        peer->send_buffer.erase(peer->send_buffer.begin(), peer->send_buffer.begin() + bytes_tx);
                    } else if (bytes_tx < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        to_disconnect.push_back(id);
                    }
                }
            }
        } // Drop the lock before calling user handler!

        // Handle received messages outside the lock
        if (msg_handler_) {
            for (const auto& pair : to_handle) {
                msg_handler_(pair.first, pair.second);
            }
        }

        // Remove dead peers
        for (uint64_t id : to_disconnect) {
            disconnect_peer(id);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU spin
    }
}

} // namespace net
} // namespace kasturisundari
