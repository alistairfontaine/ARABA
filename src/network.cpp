#include "network.hpp"
#include <linux/if_packet.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h>

namespace araba {

NetworkInterface::NetworkInterface() : sock_fd(-1), if_index(0) {
    std::memset(local_mac, 0, 6);
}

NetworkInterface::~NetworkInterface() {
    close();
}

bool NetworkInterface::open(const std::string& interface_name) {
    if (sock_fd != -1) close();

    // Create raw socket
    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd < 0) {
        std::cerr << "[Network] Error: Could not create raw socket. (Do you have sudo?)" << std::endl;
        return false;
    }

    // Get interface index
    if_index = if_nametoindex(interface_name.c_str());
    if (if_index == 0) {
        std::cerr << "[Network] Error: Interface " << interface_name << " not found." << std::endl;
        close();
        return false;
    }

    // Get local MAC
    if (!fetch_mac_from_interface(interface_name, local_mac)) {
        std::cerr << "[Network] Error: Could not get MAC for " << interface_name << std::endl;
        close();
        return false;
    }

    std::cout << "[Network] Interface opened: " << interface_name << " (Index: " << if_index << ", MAC: " << mac_to_string(local_mac) << ")" << std::endl;
    return true;
}

void NetworkInterface::close() {
    if (sock_fd != -1) {
        ::close(sock_fd);
        sock_fd = -1;
        std::cout << "[Network] Socket closed." << std::endl;
    }
}

bool NetworkInterface::fetch_mac_from_interface(const std::string& ifname, uint8_t mac[6]) {
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);

    int temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (temp_sock < 0) return false;

    if (ioctl(temp_sock, SIOCGIFHWADDR, &ifr) < 0) {
        ::close(temp_sock);
        return false;
    }

    ::close(temp_sock);

    std::memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
    return true;
}

bool NetworkInterface::send_packet(const Packet& packet, const uint8_t dest_mac[6]) {
    if (sock_fd < 0 || if_index == 0) return false;

    struct sockaddr_ll addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = if_index;
    addr.sll_halen = 6;
    std::memcpy(addr.sll_addr, dest_mac, 6);

    ssize_t sent = sendto(sock_fd, &packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr));
    if (sent < 0) {
        std::cerr << "[Network] Send failed: " << strerror(errno) << std::endl;
        return false;
    }
    // Packet sent silently
    return true;
}

bool NetworkInterface::receive_packet(Packet& packet) {
    if (sock_fd < 0) return false;

    uint8_t buffer[2048];
    struct sockaddr_ll addr;
    socklen_t addr_len = sizeof(addr);

    ssize_t bytes = recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addr_len);
    if (bytes < 0) {
        return false;
    }

    // Ethernet Header is 14 bytes: 6 (Dest) + 6 (Src) + 2 (Type)
    // We must skip these to find our custom Packet header
    const size_t ETHERNET_HEADER_SIZE = 14;

    // Check if the packet is large enough to contain Ethernet Header + Our Packet
    if (bytes < (ssize_t)(ETHERNET_HEADER_SIZE + sizeof(PacketHeader))) {
        return false;
    }

    // Calculate the offset to our custom packet
    size_t packet_offset = ETHERNET_HEADER_SIZE;

    // Copy header
    std::memcpy(&packet.header, buffer + packet_offset, sizeof(PacketHeader));

    // Check payload length sanity
    if (packet.header.payload_len > MAX_PAYLOAD_SIZE) {
        packet.header.payload_len = 0; // Ignore corrupted packets
        return false;
    }

    // Copy payload (if any)
    if (packet.header.payload_len > 0) {
        size_t total_needed = packet_offset + sizeof(PacketHeader) + packet.header.payload_len;
        if (bytes >= (ssize_t)total_needed) {
            std::memcpy(packet.payload, buffer + packet_offset + sizeof(PacketHeader), packet.header.payload_len);
        }
    }

    return true;
}

bool NetworkInterface::get_local_mac(uint8_t mac[6]) {
    if (sock_fd == -1) return false;
    std::memcpy(mac, local_mac, 6);
    return true;
}

} // namespace araba
