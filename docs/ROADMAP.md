# ARABA Project Roadmap

## Phase 1: The Core (Current)
- [x] Project Structure & Licensing
- [ ] **Packet Protocol Design**: Define the binary header, types, and checksums.
- [ ] **Serialization**: Code to pack/unpack structs to binary buffers.
- [ ] **Basic Unit Tests**: Verify packet integrity without network.

## Phase 2: The Network
- [ ] **Discovery**: Implement "Hello" broadcasting to find neighbors.
- [ ] **Routing Table**: Build the AODV (Ad-hoc On-Demand Distance Vector) logic.
- [ ] **Packet Forwarding**: Logic to pass a message from Node A -> B -> C.

## Phase 3: Reliability
- [ ] **Store & Forward**: Save packets to disk if the next hop is unreachable.
- [ ] **Encryption**: Custom AES-256 implementation for end-to-end security.
- [ ] **Concurrency**: Multi-threaded packet processing.

## Phase 4: The Interface
- [ ] **CLI Tool**: `araba send <message> <target_node>`
- [ ] **GUI (Optional)**: A simple Qt or terminal-based map of neighbors.

## Phase 5: Deployment
- [ ] **Cross-Platform**: Compile for Linux, Windows, macOS.
- [ ] **Documentation**: Full API and user guide.
- [ ] **Open Source Release**: Public GitHub launch.
