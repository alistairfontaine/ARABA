#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <string>
#include <sstream>
#include <mutex>
#include <ctime>
#include "network.hpp"
#include "packet.hpp"
#include "routing.hpp"
#include "persistence.hpp"

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
              << "  receive  - Check for messages sent to you\n"
              << "  queue    - Show pending .rue messages\n"
              << "  quit     - Stop the node\n"
              << "------------------------\n" << std::endl;
}

// Pack routes into payload for ROUTE_ADV
size_t pack_routes(const std::set<std::string>& neighbors, uint8_t* payload, size_t max_size) {
    size_t offset = 1;
    uint8_t count = 0;

    for (const auto& n : neighbors) {
        if (offset + 7 > max_size) break;
        uint8_t mac[6];
        string_to_mac(n, mac);
        std::memcpy(payload + offset, mac, 6);
        payload[offset + 6] = 1;
        offset += 7;
        count++;
    }
    payload[0] = count;
    return offset;
}

// Unpack routes from payload and add to table
void unpack_routes(const uint8_t* payload, size_t len, const uint8_t via[6], RoutingTable& table) {
    if (len < 1) return;
    uint8_t count = payload[0];
    size_t offset = 1;

    for (int i = 0; i < count; ++i) {
        if (offset + 7 > len) break;
        uint8_t dest[6];
        std::memcpy(dest, payload + offset, 6);
        uint8_t hops = payload[offset + 6];
        table.add_route(dest, via, hops + 1);
        offset += 7;
    }
}

// Network Loop — SILENT background operation
void network_loop(NetworkInterface& net, RoutingTable& table, std::set<std::string>& known_neighbors, uint32_t& seq_num, RueQueue& queue) {
    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    log_event("Network loop started on " + mac_to_string(my_mac));

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
        for (int i = 0; i < 10; ++i) {
            if (!running) break;
            if (net.receive_packet(received)) {
                if (std::memcmp(received.header.source, my_mac, 6) == 0) continue;

                std::string src_mac = mac_to_string(received.header.source);

                std::lock_guard<std::mutex> lock(net_mutex);

                // Always add direct neighbor (1 hop)
                if (known_neighbors.find(src_mac) == known_neighbors.end()) {
                    known_neighbors.insert(src_mac);
                    table.add_route(received.header.source, received.header.source, 1);
                    log_event("NEW NEIGHBOR: " + src_mac);
                }

                // Auto-deliver queued messages for this neighbor
                uint8_t src_bytes[6];
                string_to_mac(src_mac, src_bytes);
                auto pending = queue.get_pending(src_bytes);
                if (!pending.empty()) {
                    log_event("Auto-delivering " + std::to_string(pending.size()) + " queued messages to " + src_mac);
                    for (const auto& entry : pending) {
                        Packet fwd;
                        fwd.init(PacketType::DATA, my_mac, entry.dest_mac, seq_num++);
                        std::memcpy(fwd.payload, entry.payload, entry.payload_len);
                        fwd.header.payload_len = entry.payload_len;
                        net.send_packet(fwd, entry.dest_mac);
                        queue.remove_entry(entry);
                    }
                }

                // Handle DISCOVERY: Reply with ROUTE_ADV
                if (received.header.type == PacketType::DISCOVERY) {
                    Packet adv_pkt;
                    adv_pkt.init(PacketType::ROUTE_ADV, my_mac, received.header.source, seq_num++);

                    uint8_t adv_payload[512];
                    size_t adv_len = pack_routes(known_neighbors, adv_payload, 512);

                    std::memcpy(adv_pkt.payload, adv_payload, adv_len);
                    adv_pkt.header.payload_len = adv_len;
                    net.send_packet(adv_pkt, received.header.source);
                }

                // Handle ROUTE_ADV: Learn new routes
                if (received.header.type == PacketType::ROUTE_ADV) {
                    unpack_routes(received.payload, received.header.payload_len, received.header.source, table);
                    log_event("ROUTE_ADV from " + src_mac);
                }

                // Handle DATA: Forward or receive
                if (received.header.type == PacketType::DATA) {
                    if (std::memcmp(received.header.dest, my_mac, 6) != 0) {
                        const RouteEntry* route = table.find_next_hop(received.header.dest);
                        if (route && received.header.ttl > 1) {
                            Packet fwd = received;
                            fwd.header.ttl--;
                            net.send_packet(fwd, route->next_hop);
                            log_event("FORWARDED DATA to " + mac_to_string(route->next_hop));
                        }
                    } else {
                        std::string msg((char*)received.payload, received.header.payload_len);
                        log_event("MESSAGE RECEIVED from " + src_mac + ": " + msg);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    log_event("Network loop stopped.");
}

int main() {
    std::cout << "=== ARABA Node v0.6 (Store & Forward) ===\n"
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
    RueQueue queue("pending_messages.rue");

    std::thread net_thread(network_loop, std::ref(net), std::ref(my_table), std::ref(known_neighbors), std::ref(seq_num), std::ref(queue));

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
            size_t start = (lines.size() > 10) ? lines.size() - 10 : 0;
            for (size_t i = start; i < lines.size(); ++i) {
                std::cout << lines[i] << "\n";
            }
            std::cout << "-------------------------\n";
        }
        else if (command == "receive" || command == "recv") {
            std::cout << "\n--- Checking for messages ---\n";
            std::ifstream log("araba.log");
            std::string line;
            std::vector<std::string> lines;
            while (std::getline(log, line)) {
                if (line.find("MESSAGE RECEIVED") != std::string::npos) {
                    lines.push_back(line);
                }
            }
            if (lines.empty()) {
                std::cout << "No messages yet.\n";
            } else {
                for (const auto& l : lines) {
                    std::cout << l << "\n";
                }
            }
            std::cout << "-------------------------\n";
        }
        else if (command == "queue") {
            queue.print_status();
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

            uint8_t dest_mac[6];
            string_to_mac(target_mac, dest_mac);

            // Check if target is reachable
            bool reachable = false;
            {
                std::lock_guard<std::mutex> lock(net_mutex);
                if (known_neighbors.find(target_mac) != known_neighbors.end()) {
                    reachable = true;
                } else if (my_table.find_next_hop(dest_mac)) {
                    reachable = true;
                }
            }

            Packet msg_pkt;
            msg_pkt.init(PacketType::DATA, my_mac, dest_mac, seq_num++);

            size_t len = message.length();
            if (len > MAX_PAYLOAD_SIZE) len = MAX_PAYLOAD_SIZE;
            std::memcpy(msg_pkt.payload, message.c_str(), len);
            msg_pkt.header.payload_len = len;

            if (reachable) {
                {
                    std::lock_guard<std::mutex> lock(net_mutex);
                    net.send_packet(msg_pkt, dest_mac);
                }
                std::cout << "Sent to " << target_mac << ": \"" << message << "\"\n";
            } else {
                queue.enqueue(dest_mac, msg_pkt.payload, msg_pkt.header.payload_len);
                std::cout << "Target " << target_mac << " offline. Message saved to .rue queue.\n";
                std::cout << "Will auto-deliver when neighbor returns.\n";
            }
        }
        else if (!command.empty()) {
            std::cout << "Unknown command. Use: send, list, table, status, receive, queue, quit.\n";
        }
    }

    running = false;
    if (net_thread.joinable()) net_thread.join();
    net.close();
    std::cout << "\nARABA Node stopped. Log saved to araba.log\n";
    return 0;
}
