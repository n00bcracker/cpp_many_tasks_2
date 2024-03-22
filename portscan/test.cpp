#include "portscan.h"
#include "util.h"

#include <memory>
#include <vector>
#include <set>
#include <chrono>
#include <ranges>

#include <cactus/test.h>

namespace {

using namespace std::chrono_literals;

const cactus::SocketAddress kLocalhost{"127.0.0.1", 0};
constexpr uint16_t kStartPort = 25'000;
constexpr uint16_t kEndPort = 26'000;
constexpr auto kSize = kEndPort - kStartPort;

void Test(const std::set<uint16_t>& opened) {
    std::vector<std::unique_ptr<cactus::IListener>> listeners;
    for (auto port : opened) {
        auto conn = cactus::ListenTCP({kLocalhost.GetIp(), port});
        listeners.push_back(std::move(conn));
    }

    auto ports = ScanPorts(kLocalhost, kStartPort, kEndPort, 1s);

    REQUIRE(ports.size() == kSize);
    for (auto i = kStartPort; auto [port, state] : ports) {
        INFO(port);
        REQUIRE(port == i++);
        if (opened.contains(port)) {
            REQUIRE(state == PortState::OPEN);
        } else {
            REQUIRE(state == PortState::CLOSED);
        }
    }
}

void Test(auto&& range) {
    auto view = range | std::views::common;
    const std::set<uint16_t> ports{view.begin(), view.end()};
    Test(ports);
}

std::set<uint16_t> GetFreePorts() {
    std::set<uint16_t> ports;
    for (auto port : std::views::iota(kStartPort, kEndPort)) {
        try {
            cactus::ListenTCP({kLocalhost.GetIp(), port});
            ports.emplace(port);
        } catch (...) {
        }
    }
    return ports;
}

}  // namespace

FIBER_TEST_CASE("Find open ports") {
    static_assert(kSize >= 1'000);

    auto ports = GetFreePorts();
    if (ports.size() < 500) {
        FAIL("not enough free ports");
    }

    Test({});
    Test(ports | std::views::take(1));
    Test(ports | std::views::take(3));
    Test(ports | std::views::take(100));
    Test(ports | std::views::drop(300) | std::views::take(15));
    Test(ports | std::views::drop(200));
    Test(ports | std::views::filter([](auto x) { return x % 3; }));
    Test(ports);
}

FIBER_TEST_CASE("Timeout") {
    constexpr auto kFirstPort = 100;
    constexpr auto kNumPorts = 15;
    CPUTimer timer;
    auto ports = ScanPorts({"240.0.0.1", 0}, kFirstPort, kFirstPort + kNumPorts, 100ms);

    auto [wall_time, cpu_time] = timer.GetTimes();
    CHECK(wall_time > 1400ms);
    CHECK(wall_time < 2000ms);
    CHECK(cpu_time < 50ms);

    REQUIRE(ports.size() == kNumPorts);
    for (auto i = kFirstPort; auto [port, state] : ports) {
        INFO(port);
        CHECK(port == i++);
        CHECK(state == PortState::TIMEOUT);
    }
}
