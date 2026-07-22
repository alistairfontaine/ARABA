#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include "packet.hpp"

namespace araba {

class NetworkInterface {
public:
    NetworkInterface();
    ~NetworkInterface();

    // Initialize the socket on a specific interface (e.g., "wlan0", "eth0")
    bool open(const std::string& interface_name);

    // Close the socket
    void close();

    // Send a packet to a specific destination MAC
    bool send_packet(const Packet& packet, const uint8_t dest_mac[6]);

    // Receive a packet (blocks until a packet arrives or timeout)
    // Returns true if a packet was received, false on timeout/error
    // Fills 'packet' with the received data
    bool receive_packet(Packet& packet);

    // Get the MAC address of the local interface
    bool get_local_mac(uint8_t mac[6]);

    // Check if the socket is open
    bool is_open() const { return sock_fd != -1; }

private:
    int sock_fd;
    int if_index; // Store the interface index
    uint8_t local_mac[6];

    // Helper to get MAC address from interface name
    bool fetch_mac_from_interface(const std::string& ifname, uint8_t mac[6]);
};

} // namespace araba
