#pragma once

#include <string>
#include <cstdint>

namespace araba {

struct NodeConfig {
    std::string passphrase = "ARABA_MESH_SECRET_2026";
    std::string interface = "";  // Empty = auto-detect
    std::string log_file = "araba.log";
    std::string queue_file = "pending_messages.rue";
    uint32_t fragment_timeout_sec = 60;
    uint8_t discovery_ttl = 64;
    uint16_t discovery_interval_ms = 1000;
};

bool load_config(const std::string& filepath, NodeConfig& config);
void save_default_config(const std::string& filepath);

} // namespace araba
