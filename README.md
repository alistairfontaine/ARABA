<img src="assets/logo.png" alt="ARABA" width="100%">

---
# ARABA — **The Offline-First Global Mesh Network**

&gt; *"For the day the towers go dark."*

ARABA is a zero-dependency, offline-first mesh networking protocol built in pure C++17. It turns any Linux device with a Wi-Fi adapter into an encrypted message relay node — no internet, no cell towers, no infrastructure required. It is a high-performance, zero-dependency C++ mesh networking protocol designed to create a life-saving communication network when the internet is down. It turns any laptop or device with a network card into a self-healing node in a decentralized mesh.

---

## The Vision
When the grid fails, the internet dies. Starlink requires hardware and satellites. ARABA requires **nothing but code**. It is the vehicle for data in the darkest times.

---

## Features

- 🔒 **AES-256 Encryption** — Custom implementation, zero external dependencies
- 📡 **Raw Socket Mesh** — Direct device-to-device communication over Wi-Fi
- 🧠 **AODV-Lite Routing** — Automatic route discovery and multi-hop forwarding
- 💾 **Store & Forward** — Messages survive reboots and auto-deliver when neighbors return (`.rue` format)
- 🖥️ **Interactive CLI** — Clean shell with background network thread
- ⚡ **Zero Bloat** — No OpenSSL, no Boost, no external libraries
- **Zero Dependencies**: Pure C++17. No Boost, No OpenSSL (crypto is custom).
- **Delay-Tolerant**: Stores and forwards messages even if nodes are offline.
- **Self-Healing**: Automatically reroutes traffic if a node goes down.
- **Lightweight**: Runs on 20-year-old hardware.

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

---

## License
MIT License - Copyright (c) 2026 Alistair Fontaine

---
