#pragma once

#include <vector>
#include <utility>

#include <cactus/cactus.h>

enum class PortState { OPEN, CLOSED, TIMEOUT };

using Ports = std::vector<std::pair<uint16_t, PortState>>;

Ports ScanPorts(const cactus::SocketAddress& remote, uint16_t start, uint16_t end,
                cactus::Duration timeout);
