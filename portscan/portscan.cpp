#include "portscan.h"
#include <cstdint>

#include "cactus/core/timer.h"
#include "cactus/net/address.h"

PortState CheckPort(const cactus::SocketAddress& addr, const cactus::Duration timeout) {
    cactus::TimeoutGuard guard{timeout};
    try {
        auto conn = DialTCP(addr);
        conn->Close();
    } catch (const cactus::TimeoutException& ex) {
        return PortState::TIMEOUT;
    } catch (...) {
        return PortState::CLOSED;
    }

    return PortState::OPEN;
}

Ports ScanPorts(const cactus::SocketAddress& remote, uint16_t start, uint16_t end,
                cactus::Duration timeout) {
    Ports res;
    cactus::SocketAddress checking_address = remote;
    for (uint16_t port = start; port < end; ++port) {
        checking_address.SetPort(port);
        res.emplace_back(std::make_pair(port, CheckPort(checking_address, timeout)));
    }

    return res;
}
