#pragma once

#include <vector>
#include <utility>

#include <cactus/cactus.h>

enum class KnockProtocol { TCP, UDP };

using Ports = std::vector<std::pair<uint16_t, KnockProtocol>>;

void KnockPorts(const cactus::SocketAddress& remote, const Ports& ports, cactus::Duration delay);
