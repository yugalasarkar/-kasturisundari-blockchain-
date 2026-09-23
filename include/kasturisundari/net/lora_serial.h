// Kasturisundari Chain — Sovereign Physical Transport (Mesh-Radio & LoRa Bridge)
// Abstracted serial/TTY transport layer for offline LoRa / Mesh Radio consensus propagation.

#pragma once

#include "kasturisundari/net/p2p_message.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <optional>

namespace kasturisundari {
namespace net {

constexpr size_t MAX_COMPACT_FRAME_SIZE = 256;
constexpr uint8_t MESH_MAGIC_BYTE = 0x4B; // 'K' for Kasturi

/// Micro-Serialization format for radio packet framing (< 256 bytes)
struct CompactMeshFrame {
    uint8_t  magic = MESH_MAGIC_BYTE;
    uint8_t  type = 1;        // 1 = Block Approval, 2 = Tx
    uint8_t  hop_count = 0;   // Hop counter for mesh relaying
    uint16_t payload_len = 0;
    std::vector<uint8_t> payload;
    uint16_t crc16 = 0;

    std::vector<uint8_t> serialize() const;
    static std::optional<CompactMeshFrame> deserialize(const std::vector<uint8_t>& data);
    uint16_t compute_crc16() const;
};

class LoRaSerialTransport {
public:
    using FrameHandler = std::function<void(const CompactMeshFrame&)>;

    LoRaSerialTransport();
    ~LoRaSerialTransport();

    /// Open serial/TTY device (e.g., /dev/ttyUSB0, /dev/pts/X)
    bool open_device(const std::string& dev_path);
    
    /// Attach an already opened TTY file descriptor (useful for virtual loopback testing)
    bool attach_fd(int fd);

    /// Close serial connection
    void close_device();

    /// Transmit a compact frame over radio/serial transport
    bool transmit_frame(const CompactMeshFrame& frame);

    /// Set callback for received compact frames
    void set_frame_handler(FrameHandler handler);

    /// Start reader thread
    bool start_listening();
    
    /// Stop reader thread
    void stop_listening();

    bool is_open() const { return fd_ >= 0; }

private:
    int fd_{-1};
    bool owns_fd_{true};
    std::atomic<bool> running_{false};
    std::thread reader_thread_;
    FrameHandler frame_handler_;
    mutable std::mutex send_mtx_;

    void reader_loop();
};

} // namespace net
} // namespace kasturisundari
