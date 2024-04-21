#include "portknock.h"
#include <array>
#include <exception>
#include <iostream>
#include "cactus/core/scheduler.h"
#include "cactus/core/timer.h"
#include "cactus/io/view.h"
#include "cactus/net/net.h"

namespace {

void PortKnock(const cactus::SocketAddress& addr, const KnockProtocol protocol,
               const cactus::Duration timeout) {
    try {
        std::array<char, 0> buf;
        if (protocol == KnockProtocol::TCP) {
            auto conn = DialTCP(addr);
            conn->Write(cactus::View(buf));
            conn->Close();
        } else if (protocol == KnockProtocol::UDP) {
            cactus::SocketAddress localhost("127.0.0.1", 7979, true);
            auto conn = ListenUDP(localhost);
            conn->SendTo(cactus::View(buf), addr);
            conn->Close();
        }
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    cactus::SleepFor(timeout);
}

}  // namespace

void KnockPorts(const cactus::SocketAddress& remote, const Ports& ports, cactus::Duration delay) {
    cactus::SocketAddress cur_addr(remote);
    for (const auto& [port, protocol] : ports) {
        cur_addr.SetPort(port);
        PortKnock(cur_addr, protocol, delay);
    }
}
