#include "routing.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>

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

// --- Router Logic ---

Router::Router() {
    std::memset(my_mac, 0, 6); // Initialize as 00:00:00... (will be set later)
    sequence_number = 0;
}
    void Router::set_mac(const uint8_t mac[6]) {
    std::memcpy(my_mac, mac, 6);
}

void Router::receive_hello(const uint8_t neighbor_mac[6]) {
    // In a real AODV, we'd check TTL and update distances.
    // Here, we just assume the neighbor is 1 hop away.
    RoutingTable temp_table;
    // We would normally update the table with the neighbor being 1 hop away
    // For now, let's just log it
    std::cout << "[Router] Hello received from: " << mac_to_string(neighbor_mac) << std::endl;
}

bool Router::process_packet(const Packet& packet, RoutingTable& table) {
    if (!packet.isValid()) {
        std::cout << "[Router] Invalid packet dropped." << std::endl;
        return false;
    }

    // Check if we are the destination
    if (std::memcmp(packet.header.dest, my_mac, 6) == 0) {
        std::cout << "[Router] Packet received for ME! Processing data..." << std::endl;
        // In a real app, we would decrypt and process the payload here.
        return false; // Stop forwarding, we are the end
    }

    // We are not the destination. Check routing table.
    const RouteEntry* route = table.get_route(packet.header.dest);

    if (route) {
        // We have a path! Forward it.
        if (packet.header.ttl > 1) {
            Packet forward_pkt = packet;
            forward_pkt.header.ttl--; // Decrement TTL
            std::cout << "[Router] Forwarding packet to " << mac_to_string(route->next_hop)
                      << " (TTL: " << (int)forward_pkt.header.ttl << ")" << std::endl;
            // In a real app, we would send 'forward_pkt' to 'route->next_hop'
            return true;
        } else {
            std::cout << "[Router] TTL expired. Packet dropped." << std::endl;
            return false;
        }
    } else {
        // No route found! We need to broadcast a Route Request (RREQ).
        std::cout << "[Router] No route to destination. Broadcasting RREQ..." << std::endl;
        // Logic to create and send RREQ would go here.
        return false;
    }
}

} // namespace araba
