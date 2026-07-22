#include "routing.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <iostream>

namespace araba {

// Helper: Convert MAC to string key "00:1a:2b:..."
std::string RoutingTable::mac_to_key(const uint8_t mac[6]) {
    std::ostringstream oss;
    for (int i = 0; i < 6; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)mac[i];
        if (i < 5) oss << ":";
    }
    return oss.str();
}

void RoutingTable::add_route(const uint8_t dest[6], const uint8_t next_hop[6], uint8_t hops) {
    std::string key = mac_to_key(dest);

    RouteEntry& entry = table[key];

    // Only update if this is a shorter path or a new route
    if (!entry.is_valid || hops < entry.hops) {
        std::memcpy(entry.destination, dest, 6);
        std::memcpy(entry.next_hop, next_hop, 6);
        entry.hops = hops;
        entry.last_updated = std::time(nullptr);
        entry.is_valid = true;
        std::cout << "[Routing] Updated route to " << key << " (Hops: " << (int)hops << ")" << std::endl;
    }
}

const RouteEntry* RoutingTable::get_route(const uint8_t dest[6]) {
    std::string key = mac_to_key(dest);
    auto it = table.find(key);
    if (it != table.end() && it->second.is_valid) {
        return &(it->second);
    }
    return nullptr;
}

void RoutingTable::print_table() const {
    std::cout << "\n=== Routing Table ===" << std::endl;
    if (table.empty()) {
        std::cout << "Table is empty." << std::endl;
        return;
    }
    for (const auto& pair : table) {
        std::cout << "Dest: " << pair.first
                  << " | Next Hop: " << mac_to_string(pair.second.next_hop)
                  << " | Hops: " << (int)pair.second.hops << std::endl;
    }
    std::cout << "=====================\n" << std::endl;
}

// --- NEW FUNCTIONS ---

const RouteEntry* RoutingTable::find_next_hop(const uint8_t dest[6]) {
    std::string key = mac_to_key(dest);
    auto it = table.find(key);
    if (it != table.end() && it->second.is_valid && !is_stale(it->second.last_updated)) {
        return &(it->second);
    }
    return nullptr;
}

bool RoutingTable::is_stale(uint64_t timestamp) const {
    uint64_t now = std::time(nullptr);
    // If no update in 30 seconds, consider it stale
    return (now - timestamp) > 30;
}

// --- Router Logic (if needed later) ---
// We can keep this empty or add it later if we move Router logic here.
// For now, we just need the RoutingTable functions.

} // namespace araba
