# ARABA Roadmap

## Completed (v1.0 — Crisis Ready)

| Phase | Feature | Status |
|-------|---------|--------|
| 1 | Packet Protocol (binary headers, validation) | ✅ Done |
| 2 | Routing Engine (RoutingTable, RouteEntry) | ✅ Done |
| 3 | Raw Sockets (AF_PACKET, ETH_P_ALL) | ✅ Done |
| 4 | Clean Parse (skip 14-byte Ethernet header) | ✅ Done |
| 5 | Discovery Loop (broadcast DISCOVERY) | ✅ Done |
| 6 | Routing Logic (forward, TTL management) | ✅ Done |
| 7 | Interactive CLI (clean shell, no stdout spam) | ✅ Done |
| 8 | Multi-Hop Logic (ROUTE_ADV propagation) | ✅ Done |
| B | Store & Forward (.rue persistence queue) | ✅ Done |
| C | AES-256 Encryption (custom implementation) | ✅ Done |
| D | Crypto Integration (encrypt send, decrypt receive) | ✅ Done |
| F | Documentation & Polish (README, Makefile, docs) | ✅ Done |
| G | Cross-Platform Build (Makefile + CMake) | ✅ Done |
| H | Dynamic Key Exchange (per-pair keys from MACs) | ✅ Done |
| I | Message Fragmentation (split/reassemble &gt;480B) | ✅ Done |
| J | Config File (araba.conf, no hardcoded secrets) | ✅ Done |
| K | Message ACKs + Retry Logic (delivery confirmation) | ✅ Done |

## In Progress / Blocked

| Phase | Feature | Blocker |
|-------|---------|---------|
| E | Real-World Multi-Node Test | Needs second Linux device with ARABA |
| L | Android Port | Needs rooted device + Termux raw socket access |

## Future (v1.1+)

| Phase | Feature | Notes |
|-------|---------|-------|
| M | GPS Integration | Attach lat/lon to messages. Needs GPS hardware (USB module or phone NMEA stream) |
| N | Web Interface | HTTP server for non-CLI users. Embedded lightweight server |
| O | True DH Key Exchange | Replace shared passphrase with Diffie-Hellman per-session negotiation |
| P | Message Forwarding for Fragments | Currently fragmented messages are dropped if not for local node |
| Q | Message Compression | Compress large payloads before encryption |
| R | Voice Messages | Opus codec integration for audio mesh calls |
| S | File Transfer | Binary file chunking and reassembly |
| T | Mesh Visualizer | Real-time graph of network topology |
| U | Battery Optimization | Sleep/wake cycles for mobile nodes |

## Hardware Wishlist

| Device | Purpose | Status |
|--------|---------|--------|
| Second laptop (any Linux) | Multi-hop test | Needed |
| Raspberry Pi 4/5 | Portable relay node | Optional |
| Rooted Android phone | Mobile mesh node | Optional |
| USB GPS module (u-blox NEO-6M) | Location-tagged messages | Optional |
