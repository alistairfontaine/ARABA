#include <iostream>
#include "routing.hpp"

using namespace araba;

int main() {
    std::cout << "=== ARABA Routing Simulation ===" << std::endl;

    // 1. Setup MAC addresses
    uint8_t mac_a[6] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x01}; // Me (A)
    uint8_t mac_b[6] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x02}; // Neighbor (B)
    uint8_t mac_c[6] = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x03}; // Target (C)

    // 2. Create Routing Tables for A and B
    RoutingTable table_a;
    RoutingTable table_b;

    // 3. Simulate Discovery:
    // Node B tells Node A: "I know how to get to C (1 hop away from me)"
    // So for A, C is 2 hops away via B.
    std::cout << "\n[Simulation] Node B advertises route to C..." << std::endl;
    table_a.add_route(mac_c, mac_b, 2); // A -> B -> C (2 hops)
    table_b.add_route(mac_c, mac_c, 1); // B -> C (1 hop)

    // 4. Print A's table
    std::cout << "\n--- Node A's Routing Table ---" << std::endl;
    table_a.print_table();

    // 5. Create a packet from A to C
    Packet p;
    p.init(PacketType::DATA, mac_a, mac_c, 999);
    const char* msg = "Hello, C!";
    std::memcpy(p.payload, msg, std::strlen(msg));
    p.header.payload_len = std::strlen(msg);

    // 6. Process the packet through Node A's router
    Router router_a;
    // Set router's MAC to A's MAC
    router_a.set_mac(mac_a);

    std::cout << "\n--- Processing Packet ---" << std::endl;
    bool forwarded = router_a.process_packet(p, table_a);

    if (forwarded) {
        std::cout << "SUCCESS: Packet was forwarded to the next hop!" << std::endl;
    } else {
        std::cout << "FAILED: Packet was not forwarded." << std::endl;
    }

    return 0;
}
