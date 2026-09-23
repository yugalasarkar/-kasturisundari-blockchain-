// Kasturisundari Chain — Sovereign Physical Transport Implementation

#include "kasturisundari/net/lora_serial.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace kasturisundari {
namespace net {

uint16_t CompactMeshFrame::compute_crc16() const {
    uint16_t crc = 0xFFFF;
    auto update = [&crc](uint8_t b) {
        crc ^= b;
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    };
    update(magic);
    update(type);
    update(hop_count);
    update(static_cast<uint8_t>(payload_len >> 8));
    update(static_cast<uint8_t>(payload_len & 0xFF));
    for (uint8_t b : payload) update(b);
    return crc;
}

std::vector<uint8_t> CompactMeshFrame::serialize() const {
    std::vector<uint8_t> buf;
    buf.reserve(9 + payload.size());
    buf.push_back(0xAA); // Delimiter 1
    buf.push_back(0x55); // Delimiter 2
    buf.push_back(magic);
    buf.push_back(type);
    buf.push_back(hop_count);
    uint16_t len = static_cast<uint16_t>(payload.size());
    const_cast<CompactMeshFrame*>(this)->payload_len = len;
    buf.push_back(static_cast<uint8_t>(len >> 8));
    buf.push_back(static_cast<uint8_t>(len & 0xFF));
    buf.insert(buf.end(), payload.begin(), payload.end());
    
    uint16_t crc = compute_crc16();
    buf.push_back(static_cast<uint8_t>(crc >> 8));
    buf.push_back(static_cast<uint8_t>(crc & 0xFF));
    return buf;
}

std::optional<CompactMeshFrame> CompactMeshFrame::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 9) return std::nullopt;
    if (data[0] != 0xAA || data[1] != 0x55) return std::nullopt;
    if (data[2] != MESH_MAGIC_BYTE) return std::nullopt;

    CompactMeshFrame frame;
    frame.magic = data[2];
    frame.type = data[3];
    frame.hop_count = data[4];
    frame.payload_len = (static_cast<uint16_t>(data[5]) << 8) | data[6];

    if (data.size() < 7 + frame.payload_len + 2) return std::nullopt;

    frame.payload.assign(data.begin() + 7, data.begin() + 7 + frame.payload_len);
    
    uint16_t expected_crc = (static_cast<uint16_t>(data[7 + frame.payload_len]) << 8) | data[7 + frame.payload_len + 1];
    if (frame.compute_crc16() != expected_crc) {
        std::cerr << "[LoRa Mesh] CRC16 Checksum Mismatch!\n";
        return std::nullopt;
    }

    return frame;
}

LoRaSerialTransport::LoRaSerialTransport() {}

LoRaSerialTransport::~LoRaSerialTransport() {
    close_device();
}

bool LoRaSerialTransport::open_device(const std::string& dev_path) {
    fd_ = open(dev_path.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ < 0) return false;

    struct termios options;
    tcgetattr(fd_, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD | CS8);
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcsetattr(fd_, TCSANOW, &options);

    owns_fd_ = true;
    return true;
}

bool LoRaSerialTransport::attach_fd(int fd) {
    fd_ = fd;
    owns_fd_ = false;
    return true;
}

void LoRaSerialTransport::close_device() {
    stop_listening();
    if (fd_ >= 0) {
        if (owns_fd_) close(fd_);
        fd_ = -1;
    }
}

bool LoRaSerialTransport::transmit_frame(const CompactMeshFrame& frame) {
    std::lock_guard<std::mutex> lock(send_mtx_);
    if (fd_ < 0) return false;

    auto buf = frame.serialize();
    ssize_t written = write(fd_, buf.data(), buf.size());
    return written == static_cast<ssize_t>(buf.size());
}

void LoRaSerialTransport::set_frame_handler(FrameHandler handler) {
    frame_handler_ = handler;
}

bool LoRaSerialTransport::start_listening() {
    if (fd_ < 0 || running_) return false;
    running_ = true;
    reader_thread_ = std::thread(&LoRaSerialTransport::reader_loop, this);
    return true;
}

void LoRaSerialTransport::stop_listening() {
    if (!running_) return;
    running_ = false;
    if (reader_thread_.joinable()) reader_thread_.join();
}

void LoRaSerialTransport::reader_loop() {
    std::vector<uint8_t> buffer;
    uint8_t rx[128];

    while (running_) {
        if (fd_ < 0) break;
        ssize_t n = read(fd_, rx, sizeof(rx));
        if (n > 0) {
            buffer.insert(buffer.end(), rx, rx + n);
            
            // Search for frame delimiter 0xAA 0x55
            while (buffer.size() >= 9) {
                if (buffer[0] != 0xAA || buffer[1] != 0x55) {
                    buffer.erase(buffer.begin());
                    continue;
                }
                uint16_t plen = (static_cast<uint16_t>(buffer[5]) << 8) | buffer[6];
                size_t total_frame_len = 7 + plen + 2;
                if (buffer.size() < total_frame_len) break; // Wait for full frame

                std::vector<uint8_t> frame_bytes(buffer.begin(), buffer.begin() + total_frame_len);
                auto parsed = CompactMeshFrame::deserialize(frame_bytes);
                if (parsed && frame_handler_) {
                    frame_handler_(*parsed);
                }
                buffer.erase(buffer.begin(), buffer.begin() + total_frame_len);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace net
} // namespace kasturisundari
