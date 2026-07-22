#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <sstream>
#include <mutex>
#include "network.hpp"
#include "packet.hpp"
#include "routing.hpp"

using namespace araba;

// Global state for the network loop
std::mutex net_mutex;
bool running = true;

// Helper to print usage
void print_usage() {
    std::cout << "\n--- ARABA Commands ---" << std::endl;
    std::cout << "  send <mac_address> <message>" << std::endl;
    std::cout << "  list     - Show known neighbors" << std::endl;
    std::cout << "  table    - Show routing table" << std::endl;
    std::cout << "  quit     - Stop the node" << std::endl;
    std::cout << "------------------------\n" << std::endl;
}

// Network Loop Function (Runs in background)
void network_loop(NetworkInterface& net, RoutingTable& table, std::set<std::string>& known_neighbors, uint32_t& seq_num) {
    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    std::cout << "[System] Network loop started. Listening for neighbors..." << std::endl;

    while (running) {
        // 1. Broadcast Hello
        Packet hello_pkt;
        hello_pkt.init(PacketType::DISCOVERY, my_mac, broadcast, seq_num++);
        const char* msg = "ARABA Hello!";
        std::memcpy(hello_pkt.payload, msg, std::strlen(msg));
        hello_pkt.header.payload_len = std::strlen(msg);
        net.send_packet(hello_pkt, broadcast);

        // 2. Listen for packets
        Packet received;
        for (int i = 0; i < 10; ++i) { // 1 second
            if (!running) break;
            if (net.receive_packet(received)) {
                std::lock_guard<std::mutex> lock(net_mutex); // Lock for thread safety

                // Skip own packets
                if (std::memcmp(received.header.source, my_mac, 6) == 0) continue;

                std::string src_mac = mac_to_string(received.header.source);

                // Update Neighbor List
                if (known_neighbors.find(src_mac) == known_neighbors.end()) {
                    std::cout << "\n>>> NEW NEIGHBOR: " << src_mac << " <<<" << std::endl;
                    known_neighbors.insert(src_mac);
                    table.add_route(received.header.source, received.header.source, 1);
                }

                // Forward Logic (If not for us, and we have a route)
                if (std::memcmp(received.header.dest, my_mac, 6) != 0) {
                    const RouteEntry* route = table.find_next_hop(received.header.dest);
                    if (route) {
                        if (received.header.ttl > 1) {
                            Packet forward_pkt = received;
                            forward_pkt.header.ttl--;
                            // We need to send it, but we are in a lock.
                            // In a real app, we'd queue this. For now, we unlock and send.
                            // To keep it simple, we'll just log it for now or send carefully.
                            // Let's just log for safety in this demo.
                            std::cout << "[FORWARD LOG] Would forward to " << mac_to_string(route->next_hop) << std::endl;
                            // net.send_packet(forward_pkt, route->next_hop); // Uncomment to actually forward
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    std::cout << "[System] Network loop stopped." << std::endl;
}

int main() {
    std::cout << "=== ARABA Node v0.3 (Interactive) ===" << std::endl;
    std::cout << "Starting Mesh Network..." << std::endl;

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
    std::set<std::string> known_neighbors;
    uint32_t seq_num = 0;

    // Start Network Thread
    std::thread net_thread(network_loop, std::ref(net), std::ref(my_table), std::ref(known_neighbors), std::ref(seq_num));

    print_usage();

    std::string input_line;
    while (running) {
        std::cout << "araba> ";
        if (!std::getline(std::cin, input_line)) {
            running = false;
            break;
        }

        std::istringstream iss(input_line);
        std::string command;
        iss >> command;

        if (command == "quit" || command == "exit") {
            running = false;
            break;
        }
        else if (command == "list") {
            std::lock_guard<std::mutex> lock(net_mutex);
            std::cout << "Known Neighbors (" << known_neighbors.size() << "):\n";
            for (const auto& n : known_neighbors) {
                std::cout << "  - " << n << std::endl;
            }
        }
        else if (command == "table") {
            my_table.print_table();
        }
        else if (command == "send") {
            std::string target_mac;
            std::getline(iss >> std::ws, input_line); // Get the rest of the line as the message

            // Simple validation: check if target is in known neighbors
            if (known_neighbors.find(target_mac) == known_neighbors.end()) {
                std::cout << "Error: Target MAC " << target_mac << " not found in neighbors." << std::endl;
                std::cout << "Hint: Wait a few seconds for them to appear, or use 'list'." << std::endl;
                continue;
            }

            // Convert string MAC to bytes
            uint8_t dest_mac[6];
            string_to_mac(target_mac, dest_mac);

            // Create Packet
            Packet msg_pkt;
            uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; // Or direct unicast?
            // For now, we send directly to the MAC (Unicast)
            msg_pkt.init(PacketType::DATA, my_mac, dest_mac, seq_num++);

            // Copy message
            size_t len = input_line.length();
            if (len > MAX_PAYLOAD_SIZE) len = MAX_PAYLOAD_SIZE;
            std::memcpy(msg_pkt.payload, input_line.c_str(), len);
            msg_pkt.header.payload_len = len;

            // Send
            {
                std::lock_guard<std::mutex> lock(net_mutex);
                net.send_packet(msg_pkt, dest_mac);
            }
            std::cout << "Sent: \"" << input_line << "\" to " << target_mac << std::endl;
        }
        else {
            std::cout << "Unknown command. Type 'help' or 'quit'." << std::endl;
        }
    }

    // Cleanup
    running = false;
    if (net_thread.joinable()) {
        net_thread.join();
    }
    net.close();
    std::cout << "ARABA Node stopped." << std::endl;
    return 0;
}
