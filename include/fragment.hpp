#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <map>
#include <chrono>
#include "packet.hpp"

namespace araba {

constexpr uint16_t MAX_FRAG_PAYLOAD = 480;

#pragma pack(push, 1)
struct FragHeader {
    uint16_t msg_id;
    uint8_t  frag_index;
    uint8_t  frag_total;
    uint16_t total_len;
};
#pragma pack(pop)

class FragmentManager {
public:
    std::vector<Packet> fragment_message(
        const uint8_t* data,
        size_t len,
        PacketType type,
        const uint8_t src[6],
        const uint8_t dst[6],
        uint32_t& seq_num
    );

    bool reassemble_packet(
        const Packet& frag,
        uint8_t* out_buf,
        size_t max_out,
        size_t& out_len
    );

    void cleanup_stale(uint32_t max_age_seconds = 60);

private:
    struct PartialMsg {
        std::map<uint8_t, std::vector<uint8_t>> fragments;
        uint8_t total_frags = 0;
        std::chrono::steady_clock::time_point received;
    };

    std::map<uint16_t, PartialMsg> partials;
    uint16_t next_msg_id = 1;
};

} // namespace araba
