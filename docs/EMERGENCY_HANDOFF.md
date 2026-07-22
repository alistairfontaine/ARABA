# ARABA — Gemini Handoff Prompt

AUTHOR: Alistair Fontaine
REPO: https://github.com/alistairfontaine/ARABA
LICENSE: MIT
WORKING DIR: /media/mr-fontaine/R/RECOVERY/Coding/ARABA

## CRITICAL RULES

1. ZERO external dependencies. Pure C++17 only.
2. NO file linking between projects. Each project is fully independent.
3. NO bloat. Minimal code. Every line earns its place.
4. Alistair's workflow: Kate editor, full file replacement, g++ compile, sudo run.
5. When editing: Send ENTIRE file contents. No patches. No find/replace.
6. Hardware: Ubuntu Studio, HP 250 G7, Wi-Fi interface wlo1.

## BUILD

```bash
cd /media/mr-fontaine/R/RECOVERY/Coding/ARABA
make

 OR

g++ -std=c++17 -Wall -Wextra -I include -o araba_node src/araba_node.cpp src/network.cpp src/routing.cpp src/persistence.cpp src/crypto.cpp
sudo ./araba_node

| File                    | Purpose                                                              |
| ----------------------- | -------------------------------------------------------------------- |
| include/packet.hpp      | PacketType enum, PacketHeader, Packet struct, MAX\_PAYLOAD\_SIZE=496 |
| include/network.hpp     | NetworkInterface class (open, close, send, receive, get\_local\_mac) |
| include/routing.hpp     | RoutingTable, RouteEntry, find\_next\_hop, add\_route, is\_stale     |
| include/persistence.hpp | RueEntry, RueQueue (enqueue, get\_pending, remove\_entry)            |
| include/crypto.hpp      | AES256 class, derive\_key, generate\_iv                              |
| src/network.cpp         | Ethernet frame handling, AF\_PACKET raw socket                       |
| src/routing.cpp         | AODV-lite routing, MAC string conversion                             |
| src/persistence.cpp     | .rue binary file I/O                                                 |
| src/crypto.cpp          | Complete AES-256 implementation (S-box, KeyExpansion, CBC)           |
| src/araba\_node.cpp     | Interactive CLI with encrypted send/receive                          |
| src/test\_crypto.cpp    | Standalone AES-256 test                                              |
| Makefile                | Build system                                                         |
| README.md               | Project documentation                                                |
| docs/VISION.md          | Philosophy                                                           |
| docs/ROADMAP.md         | Phase tracker                                                        |
| docs/ARCHITECTURE.md    | Technical blueprint                                                  |
| docs\EMERGENCY_HANDOFF.md      | This file
|

COMPLETED PHASES
All phases A through F are complete. v0.7 is functional.

| Phase | Feature                                                            |
| ----- | ------------------------------------------------------------------ |
| 1-8   | Packet protocol, routing, sockets, discovery, CLI, multi-hop logic |
| B     | Store & Forward (.rue queue)                                       |
| C     | AES-256 from scratch                                               |
| D     | Crypto integration into live node                                  |
| F     | Documentation, Makefile, README                                    |

CURRENT STATE
araba_node.cpp v0.7 is complete and working
All sends are encrypted with AES-256-CBC
All receives are decrypted automatically
Discovery and ROUTE_ADV remain plaintext
48 neighbors detected in current environment
Queue system works, auto-delivery works

NEXT PHASES

| Phase | Feature                                | Status              |
| ----- | -------------------------------------- | ------------------- |
| E     | Real-world multi-node test             | Needs second device |
| G     | Cross-platform (CMake, Windows, macOS) | Makefile done       |
| H     | Dynamic key exchange                   | Future              |
| I     | Message fragmentation                  | Future              |

KNOWN LIMITATIONS
Multi-hop routes show 1-hop only without multiple ARABA nodes
Raw sockets require sudo
Interface hardcoded to wlo1
Shared secret is hardcoded
Linux only

COMMANDS
| Command              | Description                  |
| -------------------- | ---------------------------- |
| send <mac> <message> | Send encrypted message       |
| list                 | Show neighbors               |
| table                | Show routing table           |
| status               | Show last 10 log events      |
| receive              | Check for decrypted messages |
| queue                | Show pending .rue messages   |
| quit                 | Stop node                    |

ALISTAIR'S NOTES
Passionate, detail-oriented, prefers complete file replacements
Ubuntu Studio, Kate editor, Konsole terminal
Student complex nearby causes many Wi-Fi neighbors (~48)
Redmi 14C with Termux (unrooted) — raw sockets need root
Built Purity-FS independently — do NOT link files from it
.rue format
Project motivation: survival communication when infrastructure fails
