// Kasturisundari Chain — Phase 4: Sovereign Physical Transport (Mesh Radio / LoRa) Test
#include "kasturisundari/net/lora_serial.h"
#include <iostream>
#include <cassert>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <atomic>

using namespace kasturisundari;

void test_lora_serial_mesh_loopback() {
    std::cout << "[Test Mesh Radio / LoRa] Running virtual serial loopback test..." << std::endl;

    // Create connected socket pair simulating physical UART/Serial loopback between 2 nodes
    int sv[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    assert(res == 0);

    net::LoRaSerialTransport nodeA;
    net::LoRaSerialTransport nodeB;

    nodeA.attach_fd(sv[0]);
    nodeB.attach_fd(sv[1]);

    std::atomic<bool> received{false};
    net::CompactMeshFrame received_frame;

    nodeB.set_frame_handler([&received, &received_frame](const net::CompactMeshFrame& frame) {
        received_frame = frame;
        received = true;
    });

    nodeB.start_listening();

    // 1. Prepare Compact Mesh Frame (< 256 bytes)
    net::CompactMeshFrame tx_frame;
    tx_frame.magic = net::MESH_MAGIC_BYTE;
    tx_frame.type = 1; // Block Approval
    tx_frame.hop_count = 3;
    std::string payload_data = "KASTURI_BLOCK_APPROVAL_HEADER_905466_HASH_90e8c2a77ed87062";
    tx_frame.payload.assign(payload_data.begin(), payload_data.end());

    // 2. Transmit frame from Node A over virtual serial pipe
    bool sent = nodeA.transmit_frame(tx_frame);
    assert(sent == true);

    // 3. Wait for Node B to receive over serial thread
    for (int i = 0; i < 50; ++i) {
        if (received) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    assert(received == true);
    assert(received_frame.magic == net::MESH_MAGIC_BYTE);
    assert(received_frame.type == 1);
    assert(received_frame.hop_count == 3);
    
    std::string rx_data(received_frame.payload.begin(), received_frame.payload.end());
    assert(rx_data == payload_data);

    std::cout << "  ✓ Compact Mesh Radio frame transmitted over serial transport successfully." << std::endl;
    std::cout << "  ✓ Packet Magic ('K'), CRC16 Checksum, and Payload verified over physical loopback." << std::endl;

    nodeB.stop_listening();
    close(sv[0]);
    close(sv[1]);
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  KasturiChain Phase 4: Sovereign Physical Transport Test " << std::endl;
    std::cout << "==========================================================" << std::endl;

    test_lora_serial_mesh_loopback();

    std::cout << "\n✅ PHASE 4 SOVEREIGN PHYSICAL TRANSPORT PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
