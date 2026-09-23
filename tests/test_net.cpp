// Kasturisundari Chain — Phase 5 Networking Tests
// Tests the POSIX TCP Socket P2P implementation.

#include "kasturisundari/net/p2p_message.h"
#include "kasturisundari/net/p2p_node.h"

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace kasturisundari;
using namespace kasturisundari::net;

#define GREEN "\033[32m"
#define RED   "\033[31m"
#define CYAN  "\033[36m"
#define RESET "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    std::cout << CYAN "  [TEST] " RESET << name << "... ";

#define PASS() \
    do { std::cout << GREEN "PASSED" RESET << std::endl; ++tests_passed; } while(0)

#define FAIL(msg) \
    do { std::cout << RED "FAILED: " << msg << RESET << std::endl; ++tests_failed; } while(0)

void test_network() {
    std::cout << "\n=== P2P Network (TCP Sockets) Tests ===" << std::endl;

    TEST("Message Serialization");
    std::vector<uint8_t> payload = {1, 2, 3, 4};
    P2PMessage msg_out("ping", payload);
    auto raw_bytes = msg_out.serialize();
    
    // Header should be 20 bytes + 4 bytes payload = 24 bytes
    if (raw_bytes.size() == 24) {
        PASS();
    } else {
        FAIL("expected size 24, got " + std::to_string(raw_bytes.size()));
    }

    TEST("Message Parsing");
    P2PMessage msg_in;
    bool parsed = P2PMessage::parse(raw_bytes, msg_in);
    if (parsed && msg_in.command == "ping" && msg_in.payload == payload) {
        PASS();
    } else {
        FAIL("parsing failed");
    }

    TEST("TCP Server Node Start");
    // Use a random high port to avoid conflicts
    uint16_t test_port = 30000 + (std::chrono::steady_clock::now().time_since_epoch().count() % 10000);
    P2PNode server_node(test_port);
    
    std::string received_command;
    std::vector<uint8_t> received_payload;
    bool message_received = false;

    server_node.set_message_handler([&](uint64_t peer_id, const P2PMessage& msg) {
        received_command = msg.command;
        received_payload = msg.payload;
        message_received = true;
        
        // Send a pong back
        if (msg.command == "ping") {
            server_node.send_to_peer(peer_id, P2PMessage("pong", {9, 9, 9}));
        }
    });

    if (server_node.start()) {
        PASS();
    } else {
        FAIL("failed to bind and start server");
    }

    TEST("TCP Client Connect");
    P2PNode client_node(0); // Client doesn't bind a specific listen port for testing
    client_node.start();

    bool client_pong_received = false;
    client_node.set_message_handler([&](uint64_t peer_id, const P2PMessage& msg) {
        if (msg.command == "pong" && msg.payload == std::vector<uint8_t>{9, 9, 9}) {
            client_pong_received = true;
        }
    });

    if (client_node.connect_to_peer("127.0.0.1", test_port)) {
        PASS();
    } else {
        FAIL("client failed to connect");
    }

    // Wait for connection to establish on server side
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    TEST("Nodes are connected (Server side)");
    if (server_node.connected_peers_count() == 1) {
        PASS();
    } else {
        FAIL("server does not see peer");
    }

    TEST("Send message (Client to Server)");
    client_node.broadcast(P2PMessage("ping", {0xAA, 0xBB}));
    
    // Wait for network traversal
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if (message_received && received_command == "ping" && received_payload == std::vector<uint8_t>{0xAA, 0xBB}) {
        PASS();
    } else {
        FAIL("server did not receive correct message");
    }

    TEST("Send response (Server to Client)");
    if (client_pong_received) {
        PASS();
    } else {
        FAIL("client did not receive pong response");
    }

    TEST("Node Shutdown & Disconnect");
    client_node.stop();
    
    // Wait for server to detect disconnection
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if (server_node.connected_peers_count() == 0) {
        PASS();
    } else {
        FAIL("server still sees peer after disconnect");
    }

    server_node.stop();
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 5 Net Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_network();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
