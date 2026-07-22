# ARABA Roadmap

## Completed

| Phase | Feature | Status |
|-------|---------|--------|
| 1 | Packet Protocol (binary headers, validation) | Done |
| 2 | Routing Engine (RoutingTable, RouteEntry) | Done |
| 3 | Raw Sockets (AF_PACKET, ETH_P_ALL) | Done |
| 4 | Clean Parse (skip 14-byte Ethernet header) | Done |
| 5 | Discovery Loop (broadcast DISCOVERY) | Done |
| 6 | Routing Logic (forward, TTL management) | Done |
| 7 | Interactive CLI (clean shell, no stdout spam) | Done |
| 8 | Multi-Hop Logic (ROUTE_ADV propagation) | Done |
| B | Store & Forward (.rue persistence queue) | Done |
| C | AES-256 Encryption (custom implementation) | Done |
| D | Crypto Integration (encrypt send, decrypt receive) | Done |
| F | Documentation & Polish (README, Makefile, docs) | Done |

## In Progress

| Phase | Feature | Blocker |
|-------|---------|---------|
| E | Real-World Multi-Node Test | Needs second device |

## Future

| Phase | Feature | Notes |
|-------|---------|-------|
| G | Cross-Platform Build | Makefile done. CMake, Windows, macOS next |
| H | Dynamic Key Exchange | Replace hardcoded shared secret |
| I | Message Fragmentation | For payloads &gt; 496 bytes |
| J | GPS Integration | Attach coordinates to messages |
| K | Web Interface | HTTP server for non-CLI users |
| L | Android Port | Requires rooted device for raw sockets |
