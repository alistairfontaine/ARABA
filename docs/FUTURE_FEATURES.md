# ARABA Future Features

This document tracks everything planned for v1.1 and beyond. The project is functionally complete as of v1.0 — these are enhancements, not blockers.

## Immediate (No New Hardware)

### Fragment Forwarding
Currently, if a fragmented message arrives and the destination is not the local node, it is dropped with `DROPPED fragmented DATA — forwarding not yet supported`. Fix: re-fragment for next hop or forward individual fragments.

### Message Compression
Add zlib or lz4 compression before encryption to reduce fragment count and improve throughput.

### True Diffie-Hellman Key Exchange
Replace the shared passphrase in `araba.conf` with per-session DH key negotiation. Each node pair generates ephemeral keys, derives a session key, and uses that for the conversation. No static secret needed.

## Requires Hardware

### GPS Integration (Phase M)
Attach GPS coordinates to every message. Implementation:
- Read NMEA stream from `/dev/ttyUSB0` (USB GPS module)
- Or read from Android phone via USB tethering + GPSD
- Append `lat,lon,alt,timestamp` to message payload before encryption
- Display coordinates on receive

**Hardware needed:** u-blox NEO-6M USB GPS module (~$15) or rooted phone with GPSD

### Android Port (Phase L)
Run ARABA on rooted Android via Termux. Requires:
- Root access for raw sockets (`AF_PACKET`)
- Cross-compile with Android NDK or build in Termux
- Wi-Fi interface in monitor mode or promiscuous mode

**Hardware needed:** Rooted Android phone

### Raspberry Pi Relay
Portable battery-powered mesh relay. Install Raspberry Pi OS Lite, clone ARABA, `make`, `sudo ./araba_node`. Auto-start on boot via systemd.

**Hardware needed:** Raspberry Pi 4/5 + power bank + Wi-Fi dongle (if no built-in Wi-Fi)

## Long-Term Vision

### Voice Over Mesh (Phase R)
Integrate Opus codec for real-time audio calls over the mesh. Challenges:
- Low latency routing
- Jitter buffer for out-of-order packets
- Bandwidth management (Opus ~24kbps minimum)

### File Transfer (Phase S)
Split files into chunks, send as fragmented DATA packets, reassemble and verify with checksum. Resume interrupted transfers.

### Mesh Visualizer (Phase T)
Web-based real-time graph showing:
- All known nodes
- Link quality (signal strength)
- Active routes
- Message flow animation

Runs as separate process reading `araba.log`.

## How to Contribute

Pick a feature, open an issue, build it. ARABA is MIT licensed. No bureaucracy.

## Contact

Alistair Fontaine — built this on an HP 250 G7 in a room with too many Wi-Fi neighbors.
