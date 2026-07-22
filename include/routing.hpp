#pragma once

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cstdint>
#include <ctime>
#include "packet.hpp"

namespace araba {

// Represents a route to a destination
struct RouteEntry {
    uint8_t destination[6]; // MAC of the final destination
    uint8_t next_hop[6];    // MAC of the neighbor to send to
    uint8_t hops;           // Distance (number of hops)
    uint64_t last_updated;  // Timestamp (for aging out old routes)
    bool is_valid;

    RouteEntry() : hops(255), last_updated(0), is_valid(false) {
        std::memset(destination, 0, 6);
        std::memset(next_hop, 0, 6);
    }
};

// The Routing Table
// Maps Destination MAC -> RouteEntry
class RoutingTable {
public:
    // Add or update a route
    void add_route(const uint8_t dest[6], const uint8_t next_hop[6], uint8_t hops);

    // Get the route entry directly (legacy)
    const RouteEntry* get_route(const uint8_t dest[6]);

    // Find the next hop for a specific destination (NEW)
    const RouteEntry* find_next_hop(const uint8_t dest[6]);

    // Check if a timestamp is stale (NEW)
    bool is_stale(uint64_t timestamp) const;

    // Print the table (for debugging)
    void print_table() const;

private:
    std::map<std::string, RouteEntry> table;

    // Helper to convert MAC array to string key for the map
    static std::string mac_to_key(const uint8_t mac[6]);
};

// The Router Logic
class Router {
public:
    Router();

    // Process an incoming packet
    // Returns true if the packet was forwarded, false if it was dropped or handled locally
    bool process_packet(const Packet& packet, RoutingTable& table);

    // Simulate receiving a "Hello" packet from a neighbor
    void receive_hello(const uint8_t neighbor_mac[6]);

    // Set the router's own MAC address
    void set_mac(const uint8_t mac[6]);

private:
    uint8_t my_mac[6]; // Our own MAC address
    uint32_t sequence_number; // Our unique ID counter
};

} // namespace araba
