#include <iostream>
#include <cstring>
#include "network.hpp"
#include "routing.hpp"

using namespace araba;

int main() {
    std::cout << "=== ARABA Network Test ===" << std::endl;
    std::cout << "NOTE: This test requires ROOT privileges (sudo)." << std::endl;

    // Check if running as root
    if (geteuid() != 0) {
        std::cerr << "Error: Please run with sudo (e.g., sudo ./test_network)" << std::endl;
        return 1;
    }

    NetworkInterface net;

    // Try the specific interface found on this machine (wlo1)
    std::string iface = "wlo1";

    if (!net.open(iface)) {
        std::cout << "Interface " << iface << " not found. Trying fallbacks..." << std::endl;
        // Fallback to wlan0
        if (!net.open("wlan0")) {
            // Fallback to eth0
            if (!net.open("eth0")) {
                std::cerr << "Failed to open any interface (wlo1, wlan0, eth0)." << std::endl;
                return 1;
            }
        }
    }

    uint8_t my_mac[6];
    net.get_local_mac(my_mac);
    std::cout << "Local MAC: " << mac_to_string(my_mac) << std::endl;

    // Create a test packet
    Packet p;
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    p.init(PacketType::DISCOVERY, my_mac, broadcast, 1);

    const char* msg = "ARABA Hello!";
    std::memcpy(p.payload, msg, std::strlen(msg));
    p.header.payload_len = std::strlen(msg);

    std::cout << "Sending Discovery packet..." << std::endl;
    net.send_packet(p, broadcast);

    std::cout << "Waiting for response (3 seconds)..." << std::endl;

    // Simple timeout loop
    Packet received;
    for (int i = 0; i < 30; ++i) {
        if (net.receive_packet(received)) {
            std::cout << "Received packet! Type: " << (int)received.header.type << std::endl;
            if (received.header.payload_len > 0) {
                std::cout << "Payload: " << (char*)received.payload << std::endl;
            }
            break;
        }
        sleep(1);
    }

    net.close();
    std::cout << "Test complete." << std::endl;
    return 0;
}
