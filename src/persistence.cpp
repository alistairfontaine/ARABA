#include "persistence.hpp"
#include <ctime>

namespace araba {

RueQueue::RueQueue(const std::string& filename) : filename(filename) {
    ensure_header();
}

bool RueQueue::ensure_header() {
    std::ifstream check(filename, std::ios::binary);
    if (!check) {
        // File doesn't exist, create with header
        std::ofstream create(filename, std::ios::binary);
        if (!create) return false;
        create.write(RUE_MAGIC, 4);
        create.put(RUE_VERSION);
        uint32_t count = 0;
        create.write(reinterpret_cast<const char*>(&count), 4);
        return create.good();
    }
    // Verify existing header
    char magic[4];
    check.read(magic, 4);
    if (std::memcmp(magic, RUE_MAGIC, 4) != 0) {
        std::cerr << "[RUE] Invalid magic header. Corrupted file.\n";
        return false;
    }
    return true;
}

bool RueQueue::read_header(uint32_t& count) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    file.seekg(4); // Skip magic
    char version;
    file.get(version);
    if (version != RUE_VERSION) {
        std::cerr << "[RUE] Version mismatch.\n";
        return false;
    }
    file.read(reinterpret_cast<char*>(&count), 4);
    return file.good();
}

bool RueQueue::write_header(uint32_t count) {
    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) return false;
    file.seekp(5); // Skip magic + version
    file.write(reinterpret_cast<const char*>(&count), 4);
    return file.good();
}

bool RueQueue::enqueue(const uint8_t dest[6], const uint8_t* data, uint16_t len) {
    if (len > MAX_PAYLOAD_SIZE) len = MAX_PAYLOAD_SIZE;

    std::ofstream file(filename, std::ios::app | std::ios::binary);
    if (!file) return false;

    RueEntry entry;
    std::memcpy(entry.dest_mac, dest, 6);
    entry.payload_len = len;
    std::memcpy(entry.payload, data, len);
    entry.timestamp = static_cast<uint64_t>(std::time(nullptr));

    // Write entry
    file.write(reinterpret_cast<const char*>(entry.dest_mac), 6);
    file.write(reinterpret_cast<const char*>(&entry.payload_len), 2);
    file.write(reinterpret_cast<const char*>(entry.payload), MAX_PAYLOAD_SIZE);
    file.write(reinterpret_cast<const char*>(&entry.timestamp), 8);

    if (!file.good()) return false;

    // Update count
    uint32_t count;
    read_header(count);
    count++;
    return write_header(count);
}

std::vector<RueEntry> RueQueue::get_pending(const uint8_t dest[6]) {
    std::vector<RueEntry> result;
    std::ifstream file(filename, std::ios::binary);
    if (!file) return result;

    file.seekg(9); // Skip header

    while (file.peek() != EOF) {
        RueEntry entry;
        file.read(reinterpret_cast<char*>(entry.dest_mac), 6);
        file.read(reinterpret_cast<char*>(&entry.payload_len), 2);
        file.read(reinterpret_cast<char*>(entry.payload), MAX_PAYLOAD_SIZE);
        file.read(reinterpret_cast<char*>(&entry.timestamp), 8);

        if (!file.good()) break;

        if (std::memcmp(entry.dest_mac, dest, 6) == 0) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<RueEntry> RueQueue::get_all() {
    std::vector<RueEntry> result;
    std::ifstream file(filename, std::ios::binary);
    if (!file) return result;

    file.seekg(9);

    while (file.peek() != EOF) {
        RueEntry entry;
        file.read(reinterpret_cast<char*>(entry.dest_mac), 6);
        file.read(reinterpret_cast<char*>(&entry.payload_len), 2);
        file.read(reinterpret_cast<char*>(entry.payload), MAX_PAYLOAD_SIZE);
        file.read(reinterpret_cast<char*>(&entry.timestamp), 8);
        if (!file.good()) break;
        result.push_back(entry);
    }
    return result;
}

bool RueQueue::remove_entry(const RueEntry& target) {
    auto all = get_all();
    std::vector<RueEntry> filtered;
    bool found = false;

    for (const auto& e : all) {
        if (!found && std::memcmp(e.dest_mac, target.dest_mac, 6) == 0 &&
            e.timestamp == target.timestamp &&
            e.payload_len == target.payload_len) {
            found = true;
            continue; // Skip this one
        }
        filtered.push_back(e);
    }

    if (!found) return false;

    // Rewrite entire file
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) return false;

    file.write(RUE_MAGIC, 4);
    file.put(RUE_VERSION);
    uint32_t count = filtered.size();
    file.write(reinterpret_cast<const char*>(&count), 4);

    for (const auto& e : filtered) {
        file.write(reinterpret_cast<const char*>(e.dest_mac), 6);
        file.write(reinterpret_cast<const char*>(&e.payload_len), 2);
        file.write(reinterpret_cast<const char*>(e.payload), MAX_PAYLOAD_SIZE);
        file.write(reinterpret_cast<const char*>(&e.timestamp), 8);
    }

    return file.good();
}

void RueQueue::print_status() {
    auto all = get_all();
    std::cout << "\n=== .rue Queue Status ===\n";
    std::cout << "Total pending messages: " << all.size() << "\n";
    for (const auto& e : all) {
        std::cout << "  To: " << mac_to_string(e.dest_mac)
                  << " | Size: " << e.payload_len
                  << " | Time: " << e.timestamp << "\n";
    }
    std::cout << "=========================\n";
}

} // namespace araba
