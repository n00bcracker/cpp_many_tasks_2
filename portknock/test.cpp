#include "portknock.h"

#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <set>
#include <ranges>

#include <cactus/test.h>
#include <cactus/cactus.h>

#include <catch2/matchers/catch_matchers_range_equals.hpp>

namespace {

using namespace std::chrono_literals;

const cactus::SocketAddress kLocalhost{"127.0.0.1", 0};

auto Now() {
    return std::chrono::steady_clock::now();
}

std::vector<uint16_t> GetFreePorts() {
    constexpr uint16_t kStartPort = 25'000;
    constexpr uint16_t kEndPort = 26'000;

    std::vector<uint16_t> ports;
    for (auto port : std::views::iota(kStartPort, kEndPort)) {
        try {
            cactus::ListenTCP({kLocalhost.GetIp(), port});
            ports.push_back(port);
        } catch (...) {
        }
    }
    return ports;
}

void Check(const Ports& ports) {
    constexpr auto kDelay = 100ms;

    auto it = ports.begin();
    std::unordered_set<uint16_t> unique_ports;
    cactus::ServerGroup group;
    for (auto [port, protocol] : ports) {
        if (unique_ports.contains(port)) {
            continue;
        } else if (protocol == KnockProtocol::UDP) {
            group.Spawn([&, port] {
                auto conn = cactus::ListenUDP({"127.0.0.1", port});
                cactus::SocketAddress address;
                while (true) {
                    auto size = conn->RecvFrom(cactus::View('a'), &address);
                    CHECK(size == 0);
                    if (it == ports.end()) {
                        FAIL_CHECK("extra knock " << port);
                    } else {
                        CHECK((it++)->first == port);
                    }
                }
            });
        } else {
            group.Spawn([&, port] {
                auto lsn = cactus::ListenTCP({kLocalhost.GetIp(), port});
                while (true) {
                    lsn->Accept();
                    if (it == ports.end()) {
                        FAIL_CHECK("extra knock " << port);
                    } else {
                        CHECK((it++)->first == port);
                    }
                }
            });
        }
        unique_ports.emplace(port);
    }

    cactus::Yield();
    auto start = Now();
    KnockPorts(kLocalhost, ports, kDelay);
    auto duration = Now() - start;

    CHECK(duration > ports.size() * kDelay);
    CHECK(duration < (ports.size() + 1) * kDelay);
    CHECK(it == ports.end());
}

void CheckUDP(const std::vector<uint16_t>& ports) {
    Ports to_knock(ports.size(), {0, KnockProtocol::UDP});
    std::ranges::copy(ports, (to_knock | std::views::keys).begin());
    Check(to_knock);
}

void CheckTCP(const std::vector<uint16_t>& ports) {
    Ports to_knock(ports.size(), {0, KnockProtocol::TCP});
    std::ranges::copy(ports, (to_knock | std::views::keys).begin());
    Check(to_knock);
}

}  // namespace

FIBER_TEST_CASE("Knock ports") {
    auto p = GetFreePorts();
    if (p.size() < 100) {
        FAIL("not enough free ports");
    }

    CheckUDP({p[0]});
    CheckUDP({p[15], p[32]});
    CheckUDP({p[42], p[42], p[42]});
    CheckUDP({p[5], p[4], p[5], p[5], p[4]});

    CheckTCP({p[55]});
    CheckTCP({p[7], p[2], p[99]});
    CheckTCP({p[12], p[12]});
    CheckTCP({p[0], p[1], p[2], p[0]});
    CheckTCP({p[6], p[6], p[4], p[4], p[6]});

    Check({});
    Check({{p[5], KnockProtocol::TCP}, {p[55], KnockProtocol::UDP}});
    Check({{p[5], KnockProtocol::UDP}, {p[55], KnockProtocol::TCP}});
    Check({{p[61], KnockProtocol::UDP},
           {p[62], KnockProtocol::TCP},
           {p[63], KnockProtocol::UDP},
           {p[64], KnockProtocol::TCP}});
    Check({{p[71], KnockProtocol::TCP},
           {p[71], KnockProtocol::TCP},
           {p[72], KnockProtocol::UDP},
           {p[73], KnockProtocol::UDP}});
}
