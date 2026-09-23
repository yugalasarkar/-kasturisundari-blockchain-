// Kasturisundari Chain — P2P Message Protocol
// Defines the structure of messages sent over the TCP network.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kasturisundari {
namespace net {

// Magic bytes to identify Kasturisundari Chain network packets
constexpr uint32_t KASTURISUNDARI_MAGIC = 0x4B415354; // 'KAST'

/// Common P2P Message Commands
namespace commands {
    const std::string PING = "ping";
    const std::string PONG = "pong";
    const std::string VERSION = "version";
    const std::string VERACK = "verack";
    const std::string INV = "inv";
    const std::string GETDATA = "getdata";
    const std::string TX = "tx";
    const std::string BLOCK = "block";
}

/// A standard network message on the Kasturisundari P2P network.
struct P2PMessage {
    uint32_t magic;
    std::string command; // Up to 12 chars
    uint32_t payload_length;
    std::vector<uint8_t> payload;

    P2PMessage();
    P2PMessage(const std::string& cmd, const std::vector<uint8_t>& p_payload);

    /// Serialize message for TCP transmission.
    std::vector<uint8_t> serialize() const;

    /// Attempt to parse a message from a byte buffer.
    /// Returns true if successful, and consumes the bytes from the buffer.
    static bool parse(std::vector<uint8_t>& buffer, P2PMessage& out_msg);
};

} // namespace net
} // namespace kasturisundari
