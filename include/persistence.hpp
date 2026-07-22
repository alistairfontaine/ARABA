#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "packet.hpp"

namespace araba {

// .rue file format:
// Header: "RUE\0" (4 bytes) + version (1 byte) + entry_count (4 bytes)
// Each entry: dest_mac(6) + payload_len(2) + payload(512 max) + timestamp(8)

constexpr char RUE_MAGIC[4] = {'R', 'U', 'E', '\0'};
constexpr uint8_t RUE_VERSION = 0x01;

struct RueEntry {
    uint8_t dest_mac[6];
    uint16_t payload_len;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint64_t timestamp; // Unix timestamp when queued

    RueEntry() : payload_len(0), timestamp(0) {
        std::memset(dest_mac, 0, 6);
        std::memset(payload, 0, MAX_PAYLOAD_SIZE);
    }
};

class RueQueue {
public:
    RueQueue(const std::string& filename);
    ~RueQueue() = default;

    // Add a message to the queue
    bool enqueue(const uint8_t dest[6], const uint8_t* data, uint16_t len);

    // Get all pending messages for a specific destination
    std::vector<RueEntry> get_pending(const uint8_t dest[6]);

    // Remove a specific entry after successful delivery
    bool remove_entry(const RueEntry& entry);

    // Get all entries (for debugging)
    std::vector<RueEntry> get_all();

    // Print queue status
    void print_status();

private:
    std::string filename;
    bool ensure_header();
    bool read_header(uint32_t& count);
    bool write_header(uint32_t count);
};

} // namespace araba
