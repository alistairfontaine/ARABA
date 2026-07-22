#include "config.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace araba {

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

bool load_config(const std::string& filepath, NodeConfig& config) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (key == "passphrase") config.passphrase = val;
        else if (key == "interface") config.interface = val;
        else if (key == "log_file") config.log_file = val;
        else if (key == "queue_file") config.queue_file = val;
        else if (key == "fragment_timeout_sec") config.fragment_timeout_sec = std::stoul(val);
        else if (key == "discovery_ttl") config.discovery_ttl = static_cast<uint8_t>(std::stoul(val));
        else if (key == "discovery_interval_ms") config.discovery_interval_ms = static_cast<uint16_t>(std::stoul(val));
    }

    return true;
}

void save_default_config(const std::string& filepath) {
    std::ofstream file(filepath);
    file << "# ARABA Node Configuration\n"
         << "# Generated automatically — edit as needed\n\n"
         << "passphrase=ARABA_MESH_SECRET_2026\n"
         << "interface=\n"
         << "log_file=araba.log\n"
         << "queue_file=pending_messages.rue\n"
         << "fragment_timeout_sec=60\n"
         << "discovery_ttl=64\n"
         << "discovery_interval_ms=1000\n";
}

} // namespace araba
