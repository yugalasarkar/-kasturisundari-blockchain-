// Kasturisundari Chain — P2P Message Protocol Implementation

#include "kasturisundari/net/p2p_message.h"

#include <cstring>
#include <algorithm>

namespace kasturisundari {
namespace net {

P2PMessage::P2PMessage() 
    : magic(KASTURISUNDARI_MAGIC), payload_length(0) {}

P2PMessage::P2PMessage(const std::string& cmd, const std::vector<uint8_t>& p_payload)
    : magic(KASTURISUNDARI_MAGIC), command(cmd), payload(p_payload) {
    payload_length = static_cast<uint32_t>(payload.size());
}

std::vector<uint8_t> P2PMessage::serialize() const {
    // Header size = 4 (magic) + 12 (command) + 4 (length) = 20 bytes
    std::vector<uint8_t> buf;
    buf.reserve(20 + payload.size());

    // 1. Magic
    buf.push_back(static_cast<uint8_t>(magic >> 24));
    buf.push_back(static_cast<uint8_t>(magic >> 16));
    buf.push_back(static_cast<uint8_t>(magic >> 8));
    buf.push_back(static_cast<uint8_t>(magic));

    // 2. Command (padded to 12 bytes with nulls)
    std::string cmd = command;
    if (cmd.length() > 12) cmd = cmd.substr(0, 12);
    cmd.resize(12, '\0');
    buf.insert(buf.end(), cmd.begin(), cmd.end());

    // 3. Payload Length
    buf.push_back(static_cast<uint8_t>(payload_length >> 24));
    buf.push_back(static_cast<uint8_t>(payload_length >> 16));
    buf.push_back(static_cast<uint8_t>(payload_length >> 8));
    buf.push_back(static_cast<uint8_t>(payload_length));

    // 4. Payload
    if (!payload.empty()) {
        buf.insert(buf.end(), payload.begin(), payload.end());
    }

    return buf;
}

bool P2PMessage::parse(std::vector<uint8_t>& buffer, P2PMessage& out_msg) {
    if (buffer.size() < 20) {
        return false; // Not enough data for header
    }

    // 1. Check magic
    uint32_t m = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
    if (m != KASTURISUNDARI_MAGIC) {
        // If magic doesn't match, we are out of sync. For simplicity in this demo,
        // we'll just clear the buffer. In reality, we should scan for the magic bytes.
        buffer.clear();
        return false;
    }

    // 2. Read Payload Length
    uint32_t len = (buffer[16] << 24) | (buffer[17] << 16) | (buffer[18] << 8) | buffer[19];

    // Check if we have the full payload
    if (buffer.size() < 20 + len) {
        return false; // Wait for more data
    }

    // 3. Populate message
    out_msg.magic = m;
    
    // Extract command (strip null bytes)
    std::string cmd(buffer.begin() + 4, buffer.begin() + 16);
    cmd.erase(std::find(cmd.begin(), cmd.end(), '\0'), cmd.end());
    out_msg.command = cmd;
    
    out_msg.payload_length = len;
    
    if (len > 0) {
        out_msg.payload = std::vector<uint8_t>(buffer.begin() + 20, buffer.begin() + 20 + len);
    } else {
        out_msg.payload.clear();
    }

    // 4. Consume data from buffer
    buffer.erase(buffer.begin(), buffer.begin() + 20 + len);
    return true;
}

} // namespace net
} // namespace kasturisundari
