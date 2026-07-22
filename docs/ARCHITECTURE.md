# ARABA Architecture Blueprint

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

*This document will be updated as the code evolves.*
