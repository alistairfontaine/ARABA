#include "fragment.hpp"
#include <algorithm>

namespace araba {

std::vector<Packet> FragmentManager::fragment_message(
    const uint8_t* data,
    size_t len,
    PacketType type,
    const uint8_t src[6],
    const uint8_t dst[6],
    uint32_t& seq_num
) {
    std::vector<Packet> result;

    if (len <= MAX_FRAG_PAYLOAD) {
        Packet pkt;
        pkt.init(type, src, dst, seq_num++);
        std::memcpy(pkt.payload, data, len);
        pkt.header.payload_len = len;
        result.push_back(pkt);
        return result;
    }

    uint8_t total = static_cast<uint8_t>((len + MAX_FRAG_PAYLOAD - 1) / MAX_FRAG_PAYLOAD);
    if (total == 0) total = 1;

    uint16_t msg_id = next_msg_id++;

    for (uint8_t i = 0; i < total; ++i) {
        Packet pkt;
        pkt.init(type, src, dst, seq_num++);

        FragHeader fh;
        fh.msg_id = msg_id;
        fh.frag_index = i;
        fh.frag_total = total;
        fh.total_len = static_cast<uint16_t>(len);

        size_t offset = i * MAX_FRAG_PAYLOAD;
        size_t frag_len = MAX_FRAG_PAYLOAD;
        if (offset + frag_len > len) frag_len = len - offset;

        std::memcpy(pkt.payload, &fh, sizeof(FragHeader));
        std::memcpy(pkt.payload + sizeof(FragHeader), data + offset, frag_len);
        pkt.header.payload_len = sizeof(FragHeader) + frag_len;

        result.push_back(pkt);
    }

    return result;
}

bool FragmentManager::reassemble_packet(
    const Packet& frag,
    uint8_t* out_buf,
    size_t max_out,
    size_t& out_len
) {
    if (frag.header.payload_len < sizeof(FragHeader)) return false;

    FragHeader fh;
    std::memcpy(&fh, frag.payload, sizeof(FragHeader));

    if (fh.frag_total <= 1) return false;

    auto& partial = partials[fh.msg_id];
    partial.received = std::chrono::steady_clock::now();
    partial.total_frags = fh.frag_total;

    std::vector<uint8_t> frag_data(
        frag.payload + sizeof(FragHeader),
        frag.payload + frag.header.payload_len
    );
    partial.fragments[fh.frag_index] = std::move(frag_data);

    if (partial.fragments.size() != fh.frag_total) return false;

    if (fh.total_len > max_out) {
        // Declared size does not fit the caller's buffer — drop the message.
        partials.erase(fh.msg_id);
        return false;
    }

    // Reassemble, bounding every copy by both the caller's buffer and the
    // declared total length. A malicious or corrupt peer can advertise a small
    // total_len while sending oversized fragments; copying raw fragment sizes
    // would overflow out_buf. Clamp to fh.total_len.
    size_t offset = 0;
    for (uint8_t i = 0; i < fh.frag_total; ++i) {
        const auto& f = partial.fragments[i];
        if (offset >= fh.total_len) break;
        size_t remaining = static_cast<size_t>(fh.total_len) - offset;
        size_t copy_len = (f.size() < remaining) ? f.size() : remaining;
        std::memcpy(out_buf + offset, f.data(), copy_len);
        offset += copy_len;
    }

    out_len = offset;
    partials.erase(fh.msg_id);
    return true;
}

void FragmentManager::cleanup_stale(uint32_t max_age_seconds) {
    auto now = std::chrono::steady_clock::now();
    auto it = partials.begin();
    while (it != partials.end()) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.received
        ).count();
        if (age > max_age_seconds) {
            it = partials.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace araba
