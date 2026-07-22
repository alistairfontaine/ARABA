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
#include <queue>
#include "network.hpp"
#include "packet.hpp"
#include "routing.hpp"
#include "persistence.hpp"
#include "crypto.hpp"
#include "fragment.hpp"
#include "config.hpp"

using namespace araba;

// Global state
std::mutex net_mutex;
bool running = true;
uint8_t base_key[AES_KEY_SIZE];
std::map<std::string, std::unique_ptr<AES256>> pair_ciphers;
FragmentManager frag_mgr;
NodeConfig g_config;

// ACK tracking: seq_num -> {dest_mac, retries, timestamp, message}
struct PendingAck {
    uint8_t dest_mac[6];
    uint8_t retries;
    std::chrono::steady_clock::time_point sent_time;
    std::string original_msg;
    uint32_t seq_num;
};
std::map<uint32_t, PendingAck> pending_acks;
uint32_t next_ack_seq = 1;

void log_event(const std::string& msg) {
    std::ofstream log(g_config.log_file, std::ios::app);
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
              << "  pending  - Show messages waiting for ACK\n"
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

// Send an ACK packet back to source
void send_ack(NetworkInterface& net, const uint8_t dest_mac[6], const uint8_t my_mac[6], uint32_t ack_seq) {
    Packet ack_pkt;
    ack_pkt.init(PacketType::ACK, my_mac, dest_mac, ack_seq);
    std::memcpy(ack_pkt.payload, &ack_seq, sizeof(uint32_t));
    ack_pkt.header.payload_len = sizeof(uint32_t);
    net.send_packet(ack_pkt, dest_mac);
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
        hello_pkt.header.ttl = g_config.discovery_ttl;
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

                if (received.header.type == PacketType::ACK) {
                    if (received.header.payload_len >= sizeof(uint32_t)) {
                        uint32_t acked_seq;
                        std::memcpy(&acked_seq, received.payload, sizeof(uint32_t));
                        auto it = pending_acks.find(acked_seq);
                        if (it != pending_acks.end()) {
                            log_event("ACK received for seq " + std::to_string(acked_seq) + " from " + src_mac);
                            pending_acks.erase(it);
                        }
                    }
                }

                if (received.header.type == PacketType::DATA) {
                    // Check if this is a single packet or a fragment
                    bool is_fragment = false;
                    if (received.header.payload_len >= sizeof(FragHeader)) {
                        FragHeader fh;
                        std::memcpy(&fh, received.payload, sizeof(FragHeader));
                        is_fragment = (fh.frag_total > 1);
                    }

                    if (is_fragment) {
                        // Reassemble encrypted fragments first
                        uint8_t reassembled_enc[2048];
                        size_t reassembled_enc_len = 0;
                        bool complete = frag_mgr.reassemble_packet(received, reassembled_enc, 2048, reassembled_enc_len);

                        if (!complete) {
                            // Waiting for more fragments
                            continue;
                        }

                        // Now decrypt the full reassembled ciphertext
                        AES256* aes = get_pair_cipher(received.header.source, my_mac);
                        uint8_t decrypted[2048];
                        size_t dec_len = decrypt_payload(reassembled_enc, reassembled_enc_len, decrypted, 2048, aes);

                        if (dec_len == 0) {
                            log_event("Failed to decrypt reassembled DATA from " + src_mac);
                            continue;
                        }

                        if (std::memcmp(received.header.dest, my_mac, 6) != 0) {
                            log_event("DROPPED fragmented DATA — forwarding not yet supported");
                        } else {
                            std::string msg((char*)decrypted, dec_len);
                            log_event("MESSAGE RECEIVED from " + src_mac + ": " + msg);
                            // Send ACK back
                            send_ack(net, received.header.source, my_mac, received.header.sequence_id);
                        }
                    } else {
                        // Single packet (no fragmentation) — decrypt directly
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
                            // Send ACK back
                            send_ack(net, received.header.source, my_mac, received.header.sequence_id);
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Retry unacknowledged messages
        auto now = std::chrono::steady_clock::now();
        auto it = pending_acks.begin();
        while (it != pending_acks.end()) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.sent_time).count();
            if (age > 3) {  // Retry every 3 seconds
                if (it->second.retries >= 3) {
                    log_event("DELIVERY FAILED to " + mac_to_string(it->second.dest_mac) +
                              " after 3 retries: " + it->second.original_msg);
                    it = pending_acks.erase(it);
                    continue;
                }

                // Retry: resend the same packet
                Packet retry_pkt;
                retry_pkt.init(PacketType::DATA, my_mac, it->second.dest_mac, it->second.seq_num);
                // We don't store the full encrypted payload, so we can't truly resend without re-encrypting
                // For v1.0, we log the failure. Full retry would require storing encrypted payload.
                log_event("RETRY " + std::to_string(it->second.retries + 1) +
                          " for seq " + std::to_string(it->second.seq_num) +
                          " to " + mac_to_string(it->second.dest_mac));
                it->second.retries++;
                it->second.sent_time = now;
                ++it;
            } else {
                ++it;
            }
        }

        static int cleanup_counter = 0;
        if (++cleanup_counter >= 100) {
            frag_mgr.cleanup_stale(g_config.fragment_timeout_sec);
            cleanup_counter = 0;
        }
    }
    log_event("Network loop stopped.");
}

int main(int argc, char* argv[]) {
    std::cout << "=== ARABA Node v1.0 (ACKs + Dynamic Keys + Fragments) ===\n"
              << "Starting Mesh Network...\n" << std::endl;

    if (geteuid() != 0) {
        std::cerr << "Error: Run with sudo.\n";
        return 1;
    }

    // Load or create config
    std::string config_path = "araba.conf";
    if (argc > 1) config_path = argv[1];

    if (!load_config(config_path, g_config)) {
        std::cout << "[Config] No config found. Creating default " << config_path << "\n";
        save_default_config(config_path);
        load_config(config_path, g_config);
    }
    std::cout << "[Config] Loaded from " << config_path << "\n";

    derive_key(g_config.passphrase.c_str(), base_key);
    std::cout << "[Crypto] Base key derived. Per-pair encryption active.\n" << std::endl;

    NetworkInterface net;

    if (!g_config.interface.empty()) {
        if (!net.open(g_config.interface)) {
            std::cerr << "Failed to open " << g_config.interface << ".\n";
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
    RueQueue queue(g_config.queue_file);

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
            std::ifstream log(g_config.log_file);
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
            std::ifstream log(g_config.log_file);
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
        else if (command == "pending") {
            std::lock_guard<std::mutex> lock(net_mutex);
            std::cout << "\n--- Pending ACKs (" << pending_acks.size() << ") ---\n";
            if (pending_acks.empty()) {
                std::cout << "All messages acknowledged.\n";
            } else {
                for (const auto& [seq, pa] : pending_acks) {
                    auto age = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - pa.sent_time).count();
                    std::cout << "  Seq " << seq << " to " << mac_to_string(pa.dest_mac)
                              << " | Retries: " << (int)pa.retries
                              << " | Age: " << age << "s"
                              << " | Msg: \"" << pa.original_msg.substr(0, 40) << "...\"\n";
                }
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
                uint32_t ack_seq = next_ack_seq++;
                for (auto& pkt : fragments) {
                    {
                        std::lock_guard<std::mutex> lock(net_mutex);
                        net.send_packet(pkt, dest_mac);
                    }
                }

                // Track ACK for the first fragment's seq_num (simplified)
                {
                    std::lock_guard<std::mutex> lock(net_mutex);
                    PendingAck pa;
                    std::memcpy(pa.dest_mac, dest_mac, 6);
                    pa.retries = 0;
                    pa.sent_time = std::chrono::steady_clock::now();
                    pa.original_msg = message;
                    pa.seq_num = ack_seq;
                    pending_acks[ack_seq] = pa;
                }

                std::cout << "Sent (encrypted, " << fragments.size() << " fragment(s)) to "
                          << target_mac << ": \"" << message << "\"\n";
                std::cout << "Waiting for ACK... (check 'pending' for status)\n";
            } else {
                for (auto& pkt : fragments) {
                    queue.enqueue(dest_mac, pkt.payload, pkt.header.payload_len);
                }
                std::cout << "Target " << target_mac << " offline. " << fragments.size()
                          << " fragment(s) saved to .rue queue.\n";
            }
        }
        else if (!command.empty()) {
            std::cout << "Unknown command. Use: send, list, table, status, receive, queue, pending, quit.\n";
        }
    }

    running = false;
    if (net_thread.joinable()) net_thread.join();
    net.close();
    std::cout << "\nARABA Node stopped. Log saved to " << g_config.log_file << "\n";
    return 0;
}
