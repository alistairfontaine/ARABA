#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <sstream>
#include <mutex>
#include <ctime>
#include "network.hpp"
#include "packet.hpp"
#include "routing.hpp"

using namespace araba;

// Global state
std::mutex net_mutex;
bool running = true;

// Logger to file instead of stdout
void log_event(const std::string& msg) {
    std::ofstream log("araba.log", std::ios::app);
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    log << "[" << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << "] " << msg << "\n";
}

void print_usage() {
    std::cout << "\n--- ARABA Commands ---\n"
              << "  send <mac_address> <message>\n"
              << "  list     - Show known neighbors\n"
              << "  table    - Show routing table\n"
              << "  status   - Show last 10 log events\n"
              << "  quit     - Stop the node\n"
              << "------------------------\n" << std::endl;
}

// Network Loop — SILENT except for critical events
void network_loop(NetworkInterface& net, RoutingTable& table, std::set<std::string>& known_neighbors, uint32_t& seq_num) {
    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    log_event("Network loop started on " + mac_to_string(my_mac));

    while (running) {
        // Broadcast Hello (silent — no cout)
        Packet hello_pkt;
        hello_pkt.init(PacketType::DISCOVERY, my_mac, broadcast, seq_num++);
        const char* msg = "ARABA Hello!";
        std::memcpy(hello_pkt.payload, msg, std::strlen(msg));
        hello_pkt.header.payload_len = std::strlen(msg);
        net.send_packet(hello_pkt, broadcast);

        // Listen
        Packet received;
        for (int i = 0; i < 10; ++i) {
            if (!running) break;
            if (net.receive_packet(received)) {
                if (std::memcmp(received.header.source, my_mac, 6) == 0) continue;

                std::string src_mac = mac_to_string(received.header.source);

                std::lock_guard<std::mutex> lock(net_mutex);
                if (known_neighbors.find(src_mac) == known_neighbors.end()) {
                    known_neighbors.insert(src_mac);
                    table.add_route(received.header.source, received.header.source, 1);
                    log_event("NEW NEIGHBOR: " + src_mac);
                    // Only print to console if it's truly new — but even then, let's be quiet
                    // std::cout << "\n[+] Neighbor joined: " << src_mac << "\naraba> " << std::flush;
                }

                // Forward logic (silent)
                if (std::memcmp(received.header.dest, my_mac, 6) != 0) {
                    const RouteEntry* route = table.find_next_hop(received.header.dest);
                    if (route && received.header.ttl > 1) {
                        Packet fwd = received;
                        fwd.header.ttl--;
                        net.send_packet(fwd, route->next_hop);
                        log_event("FORWARDED to " + mac_to_string(route->next_hop));
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    log_event("Network loop stopped.");
}

int main() {
    std::cout << "=== ARABA Node v0.4 (Clean Shell) ===\n"
              << "Starting Mesh Network...\n" << std::endl;

    if (geteuid() != 0) {
        std::cerr << "Error: Run with sudo.\n";
        return 1;
    }

    NetworkInterface net;
    if (!net.open("wlo1")) {
        std::cerr << "Failed to open wlo1.\n";
        return 1;
    }

    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    std::cout << "I am: " << mac_to_string(my_mac) << "\n" << std::endl;

    RoutingTable my_table;
    std::set<std::string> known_neighbors;
    uint32_t seq_num = 0;

    std::thread net_thread(network_loop, std::ref(net), std::ref(my_table), std::ref(known_neighbors), std::ref(seq_num));

    print_usage();

    std::string input_line;
    while (running) {
        std::cout << "araba> " << std::flush;
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
                std::cout << "  - " << n << "\n";
            }
        }
        else if (command == "table") {
            my_table.print_table();
        }
        else if (command == "status") {
            std::cout << "\n--- Recent Log Events ---\n";
            std::ifstream log("araba.log");
            std::string line;
            std::vector<std::string> lines;
            while (std::getline(log, line)) lines.push_back(line);
            int start = (lines.size() > 10) ? lines.size() - 10 : 0;
            for (int i = start; i < lines.size(); ++i) {
                std::cout << lines[i] << "\n";
            }
            std::cout << "-------------------------\n";
        }
        else if (command == "send") {
            std::string target_mac;
            iss >> target_mac;

            std::string message;
            std::getline(iss >> std::ws, message);

            if (target_mac.empty() || message.empty()) {
                std::cout << "Usage: send <mac_address> <message>\n";
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(net_mutex);
                if (known_neighbors.find(target_mac) == known_neighbors.end()) {
                    std::cout << "Error: " << target_mac << " not in neighbors. Use 'list'.\n";
                    continue;
                }
            }

            uint8_t dest_mac[6];
            string_to_mac(target_mac, dest_mac);

            Packet msg_pkt;
            msg_pkt.init(PacketType::DATA, my_mac, dest_mac, seq_num++);

            size_t len = message.length();
            if (len > MAX_PAYLOAD_SIZE) len = MAX_PAYLOAD_SIZE;
            std::memcpy(msg_pkt.payload, message.c_str(), len);
            msg_pkt.header.payload_len = len;

            {
                std::lock_guard<std::mutex> lock(net_mutex);
                net.send_packet(msg_pkt, dest_mac);
            }
            std::cout << "Sent to " << target_mac << ": \"" << message << "\"\n";
        }
        else if (!command.empty()) {
            std::cout << "Unknown command. Use: send, list, table, status, quit.\n";
        }
    }

    running = false;
    if (net_thread.joinable()) net_thread.join();
    net.close();
    std::cout << "\nARABA Node stopped. Log saved to araba.log\n";
    return 0;
}
