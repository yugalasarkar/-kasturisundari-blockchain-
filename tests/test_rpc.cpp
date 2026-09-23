// Kasturisundari Chain — Phase 6 JSON-RPC Tests
// Tests the zero-dependency JSON-RPC HTTP server.

#include "kasturisundari/rpc/rpc_server.h"
#include "kasturisundari/state/state_db.h"
#include "kasturisundari/mempool/mempool.h"
#include "kasturisundari/core/blockchain.h"
#include "kasturisundari/economics/tokenomics.h"
#include "kasturisundari/crypto/address.h"
#include "kasturisundari/storage/vedic_storage.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>

using namespace kasturisundari;
using namespace kasturisundari::rpc;

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

std::string send_http_post(const std::string& json_body, uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "";

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return "";
    }

    std::string http_req = 
        "POST / HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Length: " + std::to_string(json_body.length()) + "\r\n"
        "Content-Type: application/json\r\n\r\n" + json_body;

    send(sock, http_req.c_str(), http_req.length(), 0);

    char buffer[4096];
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
    return "";
}

void test_rpc() {
    std::cout << "\n=== JSON-RPC & Web3 Tests ===" << std::endl;

    std::string db_path = "./test_rpc_db";
    std::filesystem::remove_all(db_path);
    state::StateDB state;
    state.open(db_path);
    
    // Setup a user
    crypto::KeyPair alice = crypto::generate_keypair();
    state::Account acc_alice{1'000'000ULL * economics::UNITS_PER_NILA, 0};
    state.put_account(alice.address, acc_alice);
    state.put_chain_height(42);

    mempool::Mempool mempool;
    core::Blockchain chain;
    
    std::string storage_path = "./test_storage_db";
    std::filesystem::remove_all(storage_path);
    storage::VedicStorage vstorage(storage_path);
    vstorage.init();

    // Use a random high port to avoid conflicts
    uint16_t rpc_port = 40000 + (std::chrono::steady_clock::now().time_since_epoch().count() % 10000);
    RpcServer rpc(rpc_port, state, mempool, chain, vstorage);
    
    TEST("Start RPC Server");
    if (rpc.start()) {
        PASS();
    } else {
        FAIL("failed to bind port " + std::to_string(rpc_port));
        return;
    }

    // Give server a moment to start listening
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    TEST("JSON-RPC: get_info");
    std::string req1 = "{\"jsonrpc\":\"2.0\",\"method\":\"get_info\",\"id\":\"abc\"}";
    std::string resp1 = send_http_post(req1, rpc_port);
    if (resp1.find("\"height\":42") != std::string::npos && resp1.find("\"id\":\"abc\"") != std::string::npos) {
        PASS();
    } else {
        FAIL("invalid get_info response: " + resp1);
    }

    TEST("JSON-RPC: get_balance");
    std::string req2 = "{\"jsonrpc\":\"2.0\",\"method\":\"get_balance\",\"params\":{\"address\":\"" + alice.address + "\"},\"id\":\"1\"}";
    std::string resp2 = send_http_post(req2, rpc_port);
    if (resp2.find("\"result\":\"1000000.00000000\"") != std::string::npos) {
        PASS();
    } else {
        FAIL("invalid get_balance response: " + resp2);
    }

    TEST("JSON-RPC: method not found");
    std::string req3 = "{\"jsonrpc\":\"2.0\",\"method\":\"invalid_method\",\"id\":\"1\"}";
    std::string resp3 = send_http_post(req3, rpc_port);
    if (resp3.find("\"error\"") != std::string::npos && resp3.find("-32601") != std::string::npos) {
        PASS();
    } else {
        FAIL("did not return correct error");
    }

    rpc.stop();
    state.close();
    std::filesystem::remove_all(db_path);
    std::filesystem::remove_all(storage_path);
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Kasturisundari Chain — Phase 6 RPC Tests" << std::endl;
    std::cout << "================================================" << std::endl;

    test_rpc();

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Results: " << GREEN << tests_passed << " passed" << RESET
              << ", " << (tests_failed > 0 ? RED : GREEN)
              << tests_failed << " failed" << RESET << std::endl;
    std::cout << "================================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
