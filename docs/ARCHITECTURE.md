# ARABA Architecture Blueprint

### Version 2.0.0-dev (FIPS-197 Standard Compliance Upgrade)
* **Protocol Breaking Modification:** Upgraded the core cryptographic sub-layer to fully match standard FIPS-197 AES-256-CBC specifications.
* **Interoperability Notice:** Ciphertext tracking arrays on the wire are entirely recalculated. Nodes running version `2.0.0+` cannot decode encrypted payloads broadcast by old legacy node arrays (`v1.x`), requiring full grid cluster alignment.


## Overview
ARABA is a layered protocol stack implemented in pure C++.

## Components
1.  **Packet Layer**: Defines the binary structure of data.
2.  **Routing Layer**: Manages the mesh topology and pathfinding.
3.  **Transport Layer**: Handles reliability, retransmission, and encryption.
4.  **Interface Layer**: CLI and system integration.

## Data Flow
`User Input` -> `Packet Layer` -> `Routing Layer` -> `Network Socket` -> `Neighbor Node`

## Technical Decisions
- **Language**: C++17 (No exceptions, raw pointers where possible for control).
- **Memory**: Custom allocators to prevent fragmentation in long-running daemons.
- **Concurrency**: Lock-free queues for high-throughput packet processing.

# ARABA Architecture

## Stack

Application Layer
Interactive CLI (araba_node.cpp)
Commands: send, list, table, status, receive, queue, quit
Crypto Layer
AES-256-CBC encryption/decryption
PKCS#7 padding
IV prepended to each payload
Shared secret: ARABA_MESH_SECRET_2026
Persistence Layer
.rue file format
Queue messages for offline neighbors
Auto-delivery on neighbor return
Routing Layer
AODV-lite protocol
Neighbor discovery via DISCOVERY packets
Route advertisement via ROUTE_ADV packets
Multi-hop route learning
Network Layer
AF_PACKET raw sockets
ETH_P_ALL (all Ethernet frames)
Ethernet header parsing (14 bytes)
Interface binding (wlo1)

## Packet Format

### Ethernet Header (14 bytes, handled by network.cpp)
- Destination MAC: 6 bytes
- Source MAC: 6 bytes
- EtherType: 2 bytes (0x88B5 for ARABA)

### ARABA Header (20 bytes)

Offset  Field           Size
0       version         1 byte
1       type            1 byte
2       ttl             1 byte
3-8     source          6 bytes
9-14    destination     6 bytes
15-16   payload_len     2 bytes
17-20   seq_num         4 bytes

### Payload
- Max 496 bytes
- For DATA packets: IV (16 bytes) + ciphertext
- For DISCOVERY/ROUTE_ADV: plaintext

## .rue File Format

Offset  Field           Size
0-3     magic           4 bytes ("RUE\0")
4       version         1 byte
5-8     entry_count     4 bytes
9+      entries...
Entry:
0-5     dest_mac        6 bytes
6-7     payload_len     2 bytes
8-503   payload         496 bytes
504-511 timestamp       8 bytes

## Threading Model

Main Thread:
- Reads stdin
- Parses commands
- Prints results

Network Thread:
- Broadcasts DISCOVERY
- Receives packets
- Updates routing table
- Handles encryption/decryption
- Auto-delivers queued messages

Mutex: net_mutex protects known_neighbors, routing table, and queue access.

## Security Model

- Symmetric encryption (all nodes share same passphrase)
- End-to-end: intermediate forwarders cannot read payload
- Discovery/route advertisement plaintext (contains no sensitive data)
- No authentication yet (any node with the secret can join)
