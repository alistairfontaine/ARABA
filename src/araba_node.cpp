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
#include <iomanip>
#include <memory>
#include <unistd.h>
#include "network.hpp"
#include "packet.hpp"
#include "routing.hpp"
#include "persistence.hpp"
#include "crypto.hpp"
#include "fragment.hpp"

using namespace araba;

// Global state
std::mutex net_mutex;
bool running = true;
uint8_t base_key[AES_KEY_SIZE];
std::map<std::string, std::unique_ptr<AES256>> pair_ciphers;
FragmentManager frag_mgr;

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

size_t pad_data(const uint8_t* input, size_t len, uint8_t* output, size_t max_out) {
    size_t padded_len = ((len + AES_BLOCK_SIZE) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    if (padded_len > max_out) padded_len = max_out;
    std::memcpy(output, input, len);
    uint8_t pad_byte = padded_len - len;
    for (size_t i = len; i < padded_len; i++) output[i] = pad_byte;
    return padded_len;
}

size_t unpad_data(const uint8_t* input, size_t len, uint8_t* output, size_t max_out) {
    if (len == 0 || len % AES_BLOCK_SIZE != 0) return 0;
    uint8_t pad_byte = input[len - 1];
    if (pad_byte > AES_BLOCK_SIZE || pad_byte == 0) return 0;
    size_t out_len = len - pad_byte;
    if (out_len > max_out) out_len = max_out;
    std::memcpy(output, input, out_len);
    return out_len;
}

AES256* get_pair_cipher(const uint8_t their_mac[6], const uint8_t my_mac[6]) {
    std::string key = mac_to_string(their_mac);

    std::lock_guard<std::mutex> lock(net_mutex);
    auto it = pair_ciphers.find(key);
    if (it != pair_ciphers.end()) {
        return it->second.get();
    }

    uint8_t pair_key[AES_KEY_SIZE];
    derive_pair_key(my_mac, their_mac, base_key, pair_key);

    auto aes = std::make_unique<AES256>(pair_key);
    AES256* ptr = aes.get();
    pair_ciphers[key] = std::move(aes);
    return ptr;
}

size_t encrypt_payload(const uint8_t* plaintext, size_t plain_len, uint8_t* out, size_t max_out,
                       AES256* aes) {
    uint8_t iv[AES_BLOCK_SIZE];
    generate_iv(iv);

    std::memcpy(out, iv, AES_BLOCK_SIZE);

    uint8_t padded[1024];
    size_t padded_len = pad_data(plaintext, plain_len, padded, 1024);
    if (padded_len + AES_BLOCK_SIZE > max_out) return 0;

    std::memcpy(out + AES_BLOCK_SIZE, padded, padded_len);
    aes->encrypt_cbc(out + AES_BLOCK_SIZE, padded_len, iv);

    return AES_BLOCK_SIZE + padded_len;
}

size_t decrypt_payload(const uint8_t* encrypted, size_t enc_len, uint8_t* out, size_t max_out,
                       AES256* aes) {
    if (enc_len < AES_BLOCK_SIZE + AES_BLOCK_SIZE) return 0;

    uint8_t iv[AES_BLOCK_SIZE];
    std::memcpy(iv, encrypted, AES_BLOCK_SIZE);

    size_t cipher_len = enc_len - AES_BLOCK_SIZE;
    uint8_t decrypted[1024];
    std::memcpy(decrypted, encrypted + AES_BLOCK_SIZE, cipher_len);
    aes->decrypt_cbc(decrypted, cipher_len, iv);

    return unpad_data(decrypted, cipher_len, out, max_out);
}

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

void network_loop(NetworkInterface& net, RoutingTable& table, std::set<std::string>& known_neighbors,
                  uint32_t& seq_num, RueQueue& queue) {
    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    log_event("Network loop started on " + mac_to_string(my_mac));

    while (running) {
        Packet hello_pkt;
        hello_pkt.init(PacketType::DISCOVERY, my_mac, broadcast, seq_num++);
        const char* msg = "ARABA Hello!";
        std::memcpy(hello_pkt.payload, msg, std::strlen(msg));
        hello_pkt.header.payload_len = std::strlen(msg);
        net.send_packet(hello_pkt, broadcast);

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
                }

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

                if (received.header.type == PacketType::DISCOVERY) {
                    Packet adv_pkt;
                    adv_pkt.init(PacketType::ROUTE_ADV, my_mac, received.header.source, seq_num++);

                    uint8_t adv_payload[512];
                    size_t adv_len = pack_routes(known_neighbors, adv_payload, 512);

                    std::memcpy(adv_pkt.payload, adv_payload, adv_len);
                    adv_pkt.header.payload_len = adv_len;
                    net.send_packet(adv_pkt, received.header.source);
                }

                if (received.header.type == PacketType::ROUTE_ADV) {
                    unpack_routes(received.payload, received.header.payload_len, received.header.source, table);
                    log_event("ROUTE_ADV from " + src_mac);
                }

                if (received.header.type == PacketType::DATA) {
                    AES256* aes = get_pair_cipher(received.header.source, my_mac);

                    uint8_t decrypted[1024];
                    size_t dec_len = decrypt_payload(received.payload, received.header.payload_len,
                                                     decrypted, 1024, aes);

                    if (dec_len == 0) {
                        log_event("Failed to decrypt DATA from " + src_mac);
                        continue;
                    }

                    if (std::memcmp(received.header.dest, my_mac, 6) != 0) {
                        const RouteEntry* route = table.find_next_hop(received.header.dest);
                        if (route && received.header.ttl > 1) {
                            Packet fwd = received;
                            fwd.header.ttl--;
                            net.send_packet(fwd, route->next_hop);
                            log_event("FORWARDED DATA to " + mac_to_string(route->next_hop));
                        }
                    } else {
                        std::string msg((char*)decrypted, dec_len);
                        log_event("MESSAGE RECEIVED from " + src_mac + ": " + msg);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        static int cleanup_counter = 0;
        if (++cleanup_counter >= 100) {
            frag_mgr.cleanup_stale();
            cleanup_counter = 0;
        }
    }
    log_event("Network loop stopped.");
}

int main(int argc, char* argv[]) {
    std::cout << "=== ARABA Node v0.8 (Dynamic Keys + Fragments) ===\n"
              << "Starting Mesh Network...\n" << std::endl;

    if (geteuid() != 0) {
        std::cerr << "Error: Run with sudo.\n";
        return 1;
    }

    derive_key("ARABA_MESH_SECRET_2026", base_key);
    std::cout << "[Crypto] Base key derived. Per-pair encryption active.\n" << std::endl;

    NetworkInterface net;

    if (argc > 1) {
        if (!net.open(argv[1])) {
            std::cerr << "Failed to open " << argv[1] << ".\n";
            return 1;
        }
    } else {
        if (!net.open_auto()) {
            std::cerr << "Failed to auto-detect interface. Try: sudo ./araba_node wlo1\n";
            return 1;
        }
    }

    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    std::cout << "I am: " << mac_to_string(my_mac) << "\n" << std::endl;

    RoutingTable my_table;
    std::set<std::string> known_neighbors;
    uint32_t seq_num = 0;
    RueQueue queue("pending_messages.rue");

    std::thread net_thread(network_loop, std::ref(net), std::ref(my_table), std::ref(known_neighbors),
                          std::ref(seq_num), std::ref(queue));

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

            bool reachable = false;
            {
                std::lock_guard<std::mutex> lock(net_mutex);
                if (known_neighbors.find(target_mac) != known_neighbors.end()) {
                    reachable = true;
                } else if (my_table.find_next_hop(dest_mac)) {
                    reachable = true;
                }
            }

            AES256* aes = get_pair_cipher(dest_mac, my_mac);

            uint8_t encrypted[1024];
            size_t enc_len = encrypt_payload((uint8_t*)message.c_str(), message.length(),
                                             encrypted, 1024, aes);
            if (enc_len == 0) {
                std::cout << "Error: Encryption failed.\n";
                continue;
            }

            auto fragments = frag_mgr.fragment_message(encrypted, enc_len, PacketType::DATA,
                                                       my_mac, dest_mac, seq_num);

            if (reachable) {
                for (auto& pkt : fragments) {
                    {
                        std::lock_guard<std::mutex> lock(net_mutex);
                        net.send_packet(pkt, dest_mac);
                    }
                }
                std::cout << "Sent (encrypted, " << fragments.size() << " fragment(s)) to "
                          << target_mac << ": \"" << message << "\"\n";
            } else {
                for (auto& pkt : fragments) {
                    queue.enqueue(dest_mac, pkt.payload, pkt.header.payload_len);
                }
                std::cout << "Target " << target_mac << " offline. " << fragments.size()
                          << " fragment(s) saved to .rue queue.\n";
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
