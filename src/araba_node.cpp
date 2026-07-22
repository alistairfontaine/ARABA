#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <algorithm>
#include "network.hpp"
#include "packet.hpp"
#include "routing.hpp"

using namespace araba;

// Simple structure to track a neighbor
struct Neighbor {
    uint8_t mac[6];
    uint64_t last_seen;
    std::string mac_str() const {
        return mac_to_string(mac);
    }
};

int main() {
    std::cout << "=== ARABA Node v0.2 (Router) ===" << std::endl;
    std::cout << "Starting Mesh Routing Engine..." << std::endl;

    if (geteuid() != 0) {
        std::cerr << "Error: Run with sudo." << std::endl;
        return 1;
    }

    NetworkInterface net;
    std::string iface = "wlo1";

    if (!net.open(iface)) {
        std::cerr << "Failed to open interface." << std::endl;
        return 1;
    }

    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    std::cout << "I am: " << mac_to_string(my_mac) << std::endl;

    RoutingTable my_table;
    uint32_t seq_num = 0;
    std::set<std::string> known_neighbors;

    std::cout << "\n--- Mesh Routing Started ---" << std::endl;
    std::cout << "Broadcasting 'Hello' and listening for traffic..." << std::endl;

    while (true) {
        // 1. Broadcast Hello (Discovery)
        Packet hello_pkt;
        uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        hello_pkt.init(PacketType::DISCOVERY, my_mac, broadcast, seq_num++);

        const char* msg = "ARABA Hello!";
        std::memcpy(hello_pkt.payload, msg, std::strlen(msg));
        hello_pkt.header.payload_len = std::strlen(msg);

        net.send_packet(hello_pkt, broadcast);

        // 2. Listen for packets
        Packet received;
        // We listen for a bit longer to catch more traffic
        for (int i = 0; i < 10; ++i) { // 1 second
            if (net.receive_packet(received)) {
                // Skip our own packets (loopback)
                if (std::memcmp(received.header.source, my_mac, 6) == 0) {
                    continue;
                }

                // --- LOGIC: UPDATE ROUTING TABLE ---
                // If we see a packet from a neighbor, we know they exist.
                // We add a route to them (1 hop away).
                std::string src_mac = mac_to_string(received.header.source);
                if (known_neighbors.find(src_mac) == known_neighbors.end()) {
                    std::cout << "\n>>> NEW NEIGHBOR: " << src_mac << " <<<" << std::endl;
                    known_neighbors.insert(src_mac);
                    // Add route: Destination = Neighbor, Next Hop = Neighbor, Hops = 1
                    my_table.add_route(received.header.source, received.header.source, 1);
                }

                // --- LOGIC: FORWARD PACKETS ---
                // If the packet is NOT for us, and we have a route, forward it!
                if (std::memcmp(received.header.dest, my_mac, 6) != 0) {
                    // We are not the destination. Check if we know how to get there.
                    const RouteEntry* route = my_table.find_next_hop(received.header.dest);

                    if (route) {
                        // We have a path! Forward it.
                        if (received.header.ttl > 1) {
                            Packet forward_pkt = received;
                            forward_pkt.header.ttl--; // Decrement TTL
                            std::cout << "[FORWARD] Sending to " << mac_to_string(route->next_hop)
                                      << " (Dest: " << mac_to_string(received.header.dest) << ")" << std::endl;
                            net.send_packet(forward_pkt, route->next_hop);
                        } else {
                            std::cout << "[DROP] TTL expired for " << mac_to_string(received.header.dest) << std::endl;
                        }
                    } else {
                        // No route? In a real AODV, we would broadcast a Route Request (RREQ).
                        // For now, we just drop it or log it.
                        // std::cout << "[RREQ] No route to " << mac_to_string(received.header.dest) << ". Broadcasting RREQ..." << std::endl;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Print status
        std::cout << "\n--- Status (Seq: " << seq_num-1 << ") ---" << std::endl;
        std::cout << "Neighbors: " << known_neighbors.size() << std::endl;
        my_table.print_table();
        std::cout << "--------------------------\n" << std::endl;
    }

    net.close();
    return 0;
}
