<img src="assets/logo.png" alt="ARABA" width="100%">

---
# ARABA
**The Offline-First Global Mesh Network**

ARABA is a high-performance, zero-dependency C++ mesh networking protocol designed to create a life-saving communication network when the internet is down. It turns any laptop or device with a network card into a self-healing node in a decentralized mesh.

---

## The Vision
When the grid fails, the internet dies. Starlink requires hardware and satellites. ARABA requires **nothing but code**. It is the vehicle for data in the darkest times.

---

## Features
- **Zero Dependencies**: Pure C++17. No Boost, No OpenSSL (crypto is custom).
- **Delay-Tolerant**: Stores and forwards messages even if nodes are offline.
- **Self-Healing**: Automatically reroutes traffic if a node goes down.
- **Lightweight**: Runs on 20-year-old hardware.

---

## Quick Start

### Prerequisites
- A Linux machine (Ubuntu, Debian, Arch, etc.)
- `g++` compiler (GCC 11+ recommended)
- CMake (optional, for advanced builds)

### Build from Source
```bash
# Clone the repo
git clone https://github.com/alistairfontaine/ARABA.git
cd ARABA

# Compile the test
g++ -std=c++17 -Wall -Wextra -o test_packet src/test_packet.cpp

# Run the test
./test_packet

# Run the Network Daemon (Coming Soon)
bash

./araba_node --interface wlan0 --mode ad-hoc

---

## License
MIT License - Copyright (c) 2026 Alistair Fontaine

---
