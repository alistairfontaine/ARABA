#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace araba {

// Packet Types
enum class PacketType : uint8_t {
    DISCOVERY = 0x01,    // "Hello, I am here"
    ROUTE_ADV = 0x02,    // "Here is who I know"
    DATA = 0x03,         // Actual message
    ACK = 0x04           // "I got it"
};

// Configuration
constexpr size_t MAX_PAYLOAD_SIZE = 512;
constexpr uint8_t PROTOCOL_VERSION = 0x01;
constexpr uint8_t DEFAULT_TTL = 64;

// The structure of a Mesh Packet
// 'pack' ensures no padding bytes are added by the compiler
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t version;       // Protocol version
    PacketType type;       // What kind of packet is this?
    uint8_t ttl;           // Time To Live (prevents infinite loops)
    uint8_t source[6];     // MAC address of sender (6 bytes)
    uint8_t dest[6];       // MAC address of receiver (6 bytes)
    uint32_t sequence_id;  // Unique ID to prevent duplicates
    uint16_t payload_len;  // Length of the data following the header
};
#pragma pack(pop)

// The full packet structure
struct Packet {
    PacketHeader header;
    uint8_t payload[MAX_PAYLOAD_SIZE];

    // Helper to initialize the packet
    void init(PacketType type, const uint8_t src[6], const uint8_t dst[6], uint32_t seq) {
        std::memset(this, 0, sizeof(*this));
        header.version = PROTOCOL_VERSION;
        header.type = type;
        header.ttl = DEFAULT_TTL;
        std::memcpy(header.source, src, 6);
        std::memcpy(header.dest, dst, 6);
        header.sequence_id = seq;
        header.payload_len = 0;
    }

    // Helper to check if packet is valid
    bool isValid() const {
        return header.version == PROTOCOL_VERSION &&
               (header.type >= PacketType::DISCOVERY && header.type <= PacketType::ACK) &&
               header.payload_len <= MAX_PAYLOAD_SIZE;
    }
};

// Utility: MAC to String
inline std::string mac_to_string(const uint8_t mac[6]) {
    std::ostringstream oss;
    for (int i = 0; i < 6; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)mac[i];
        if (i < 5) oss << ":";
    }
    return oss.str();
}

// Utility: String to MAC
inline void string_to_mac(const std::string& mac_str, uint8_t mac_out[6]) {
    std::istringstream iss(mac_str);
    char sep;
    int temp_val; // Temporary variable to hold the integer
    for (int i = 0; i < 6; ++i) {
        if (i == 2 || i == 4 || i == 5) { // Skip colons
            iss >> sep;
        }
        iss >> std::hex >> temp_val; // Read into temp
        mac_out[i] = static_cast<uint8_t>(temp_val); // Cast and store
    }
}

} // namespace araba
