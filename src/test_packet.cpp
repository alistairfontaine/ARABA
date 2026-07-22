#include <iostream>
#include <iomanip>
#include "../include/packet.hpp"

using namespace araba;

int main() {
    std::cout << "=== ARABA Packet Test ===" << std::endl;

    Packet p;
    uint8_t src[6] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E};
    uint8_t dst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    p.init(PacketType::DATA, src, dst, 12345);

    std::cout << "Packet Created Successfully!" << std::endl;
    std::cout << "Source: " << mac_to_string(p.header.source) << std::endl;
    std::cout << "Dest: " << mac_to_string(p.header.dest) << std::endl;
    std::cout << "Type: " << (int)p.header.type << std::endl;
    std::cout << "TTL: " << (int)p.header.ttl << std::endl;
    std::cout << "Size: " << sizeof(p) << " bytes" << std::endl;

    if (p.isValid()) {
        std::cout << "Validation: PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "Validation: FAILED" << std::endl;
        return 1;
    }
}
