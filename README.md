<img src="assets/logo.png" alt="ARABA" width="100%">

### Version 2.0.0-dev (FIPS-197 Standard Compliance Upgrade)
* **Protocol Breaking Modification:** Upgraded the core cryptographic sub-layer to fully match standard FIPS-197 AES-256-CBC specifications.
* **Interoperability Notice:** Ciphertext tracking arrays on the wire are entirely recalculated. Nodes running version `2.0.0+` cannot decode encrypted payloads broadcast by old legacy node arrays (`v1.x`), requiring full grid cluster alignment.

---
# ARABA — **The Offline-First Global Mesh Network**

&gt; *"For the day the towers go dark."*

ARABA is a zero-dependency, offline-first mesh networking protocol built in pure C++17. It turns any Linux device with a Wi-Fi adapter into an encrypted message relay node — no internet, no cell towers, no infrastructure required. It is a high-performance, zero-dependency C++ mesh networking protocol designed to create a life-saving communication network when the internet is down. It turns any laptop or device with a network card into a self-healing node in a decentralized mesh.

---

# ARABA (v1.0.0 Crisis-Ready Mesh Network)

An advanced, high-performance, completely dependency-free Offline-First Mesh Network Node and Cryptographic Messaging Engine engineered from bedrock principles in pure C++17.

Designed specifically for communication finality during total infrastructure collapse, regional network blackouts, or tactical isolation scenarios, ARABA operates directly over bare-metal wireless link layers (`AF_PACKET` / `SOCK_RAW`) without relying on cellular towers, internet gateways, DNS tracking, or central routing server arrays.

---

## 🔬 Subsystem Architectural Strengths

*   **Sovereign Link-Layer Ingress:** Binds natively to raw Wi-Fi interfaces (such as `wlo1`), parsing custom link-layer Ethernet frames bypassing standard network stack overhead.
*   **Per-Pair Dynamic Encryption:** Custom dependency-free AES-256-CBC cryptographic layout where distinct pairs derive unique cryptographic keys based on mutual hardware MAC footprints combined with a base master secret passphrase.
*   **Volatile Byte Segmentation:** Automatically splits payloads exceeding 480 bytes into isolated chunks, handles message transmission queues, tracks validation ACKs, and dynamically reassembles packet buffers on receipt.
*   **Store-and-Forward Persistence:** Implements our custom `.rue` data persistence engine to automatically cache outgoing payloads to disk when a target node goes out of range, auto-delivering packets the exact moment the destination beacon drops back online.
*   **Topography Learning:** Continuous background discovery loops (`DISCOVERY`) and route propagation alerts (`ROUTE_ADV`) running an in-memory AODV-lite matrix to calculate multi-hop message forwarding lanes.

---

## 🛠️ Verification and Compilation Guide

### 📦 1. Clone the Complete Workspace Repository
```bash
git clone https://github.com
cd ARABA
```

### 🔨 2. Execute the Automated Makefile Build Script
```bash
make clean
make
```

### 🕹️ 3. Launch the Root Daemon CLI Console
```bash
sudo ./araba_node
```

---

## 💻 Native Interactive Shell Commands

*   `send <mac_address> <message>` - Encrypts, fragments, and dispatches text payloads across the mesh lanes.
*   `list`                         - Displays all active direct hardware nodes currently within RF radio sight boundaries.
*   `table`                        - Prints the parsed multi-hop AODV routing layout matrix tracking hop metrics.
*   `status`                       - Outputs a trace read block of the last 10 silent background event logs.
*   `receive`                      - Displays the secure ledger of decrypted text messages explicitly received by this node.
*   `queue`                        - Inspects the persistent offline `.rue` message delivery cache layers.
*   `quit / exit`                  - Safely shuts down running threads, unbinds sockets, and drops the node offline.

---

## 📜 Sovereign Open-Source License
ARABA is an open-source, civilian defense communication protocol distributed under the terms of the official MIT License guidelines.


## The Vision
When the grid fails, the internet dies. Starlink requires hardware and satellites. ARABA requires **nothing but code**. It is the vehicle for data in the darkest times.

---

# ARABA — Offline-First Encrypted Mesh Network
**v1.0 — Crisis Ready**

&gt; *"For the day the towers go dark."*

ARABA is a zero-dependency, offline-first mesh networking protocol built in pure C++17. It turns any Linux device with a Wi-Fi adapter into an encrypted message relay node — no internet, no cell towers, no infrastructure required.

## Features

- 🔒 **AES-256 Encryption** — Custom implementation, zero external dependencies
- 🔑 **Per-Pair Dynamic Keys** — Each node pair uses a unique derived key
- 📡 **Raw Socket Mesh** — Direct device-to-device communication over Wi-Fi
- 🧠 **AODV-Lite Routing** — Automatic route discovery and multi-hop forwarding
- 💾 **Store & Forward** — Messages survive reboots and auto-deliver when neighbors return (`.rue` format)
- ✂️ **Message Fragmentation** — Large messages split into multiple packets, reassembled on receive
- 🖥️ **Interactive CLI** — Clean shell with background network thread
- ⚙️ **Config File** — `araba.conf` for passphrase, interface, and tuning
- ⚡ **Zero Bloat** — No OpenSSL, no Boost, no external libraries
- **Zero Dependencies**: Pure C++17. No Boost, No OpenSSL (crypto is custom).
- **Delay-Tolerant**: Stores and forwards messages even if nodes are offline.
- **Self-Healing**: Automatically reroutes traffic if a node goes down.
- **Lightweight**: Runs on 20-year-old hardware.
- ✅ **Message ACKs + Retry** — Delivery confirmation with automatic retry

## Hardware Tested

- Ubuntu Studio 22.04
- HP 250 G7 Notebook
- Intel Wi-Fi interface (`wlo1`)

---

## Quick Start

### Prerequisites
- A Linux machine (Ubuntu, Debian, Arch, etc.)
- `g++` compiler (GCC 11+ recommended)
- CMake (optional, for advanced builds)

## Build

```bash
# Clone the repo
git clone https://github.com/alistairfontaine/ARABA.git
cd ARABA

# Compile the test
g++ -std=c++17 -Wall -Wextra -o test_packet src/test_packet.cpp

# Official Compilation
g++ -std=c++17 -Wall -Wextra -I include -o araba_node src/araba_node.cpp src/network.cpp src/routing.cpp src/persistence.cpp src/crypto.cpp

# Run the test
./test_packet

# Run the Network Daemon (Coming Soon)
bash

./araba_node --interface wlan0 --mode ad-hoc

### Option 1: Makefile
```bash
cd /media/mr-fontaine/R/RECOVERY/Coding/ARABA
make
sudo ./araba_node

---

## License
MIT License - Copyright (c) 2026 Alistair Fontaine

---
