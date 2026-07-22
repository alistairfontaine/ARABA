#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <algorithm>
#include "network.hpp"
#include "packet.hpp"

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
    std::cout << "=== ARABA Node v0.1 ===" << std::endl;
    std::cout << "Starting Discovery Loop..." << std::endl;

    // Check root
    if (geteuid() != 0) {
        std::cerr << "Error: Run with sudo." << std::endl;
        return 1;
    }

    NetworkInterface net;
    std::string iface = "wlo1"; // Default, can be passed as arg later

    if (!net.open(iface)) {
        std::cerr << "Failed to open interface." << std::endl;
        return 1;
    }

    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    std::cout << "I am: " << mac_to_string(my_mac) << std::endl;

    std::set<std::string> known_neighbors;
    uint32_t seq_num = 0;

    std::cout << "\n--- Starting Discovery ---" << std::endl;
    std::cout << "Broadcasting 'Hello' every 2 seconds. Listening for neighbors..." << std::endl;

    while (true) {
        // 1. Broadcast Hello
        Packet hello_pkt;
        uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        hello_pkt.init(PacketType::DISCOVERY, my_mac, broadcast, seq_num++);

        const char* msg = "ARABA Hello!";
        std::memcpy(hello_pkt.payload, msg, std::strlen(msg));
        hello_pkt.header.payload_len = std::strlen(msg);

        net.send_packet(hello_pkt, broadcast);
        std::cout << "[Sent] Hello (Seq: " << seq_num-1 << ")" << std::endl;

        // 2. Listen for 2 seconds (non-blocking loop)
        for (int i = 0; i < 20; ++i) { // 20 * 100ms = 2 seconds
            Packet received;
            if (net.receive_packet(received)) {
                // Ignore if it's our own packet (loopback)
                if (std::memcmp(received.header.source, my_mac, 6) == 0) {
                    continue;
                }

                // Process Discovery Packet
                if (received.header.type == PacketType::DISCOVERY) {
                    std::string neighbor_mac = mac_to_string(received.header.source);

                    if (known_neighbors.find(neighbor_mac) == known_neighbors.end()) {
                        // New Neighbor!
                        std::cout << "\n>>> NEW NEIGHBOR DETECTED: " << neighbor_mac << " <<<" << std::endl;
                        if (received.header.payload_len > 0) {
                            std::cout << "    Message: " << (char*)received.payload << std::endl;
                        }
                        known_neighbors.insert(neighbor_mac);
                    } else {
                        // Already known, just update "last seen" (logic could go here)
                        // std::cout << "[Ping] Heard from " << neighbor_mac << " again." << std::endl;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Print current list
        if (!known_neighbors.empty()) {
            std::cout << "\n--- Current Neighbors (" << known_neighbors.size() << ") ---" << std::endl;
            for (const auto& n : known_neighbors) {
                std::cout << "  - " << n << std::endl;
            }
            std::cout << "--------------------------------" << std::endl;
        }
    }

    net.close();
    return 0;
}
