#include "socksd.h"
#include "util.h"

#include <ranges>
#include <memory>
#include <string_view>
#include <random>
#include <array>
#include <system_error>

#include <cactus/test.h>

namespace {

using namespace std::chrono_literals;

const cactus::SocketAddress kServerAddress{"127.0.0.1", 58'433};

// Using fake client. Implementing real client is a separate task,
// and we don't want to show our solution here.
constexpr std::string_view kFakeRequest = {"\x04\x01\xe4\x41\x7f\x00\x00\x01prime\0", 14};
constexpr char kOkReply[] = {0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr char kErrorReply[] = {0x00, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

class EchoServer {
public:
    explicit EchoServer(const cactus::SocketAddress& at) : lsn_{cactus::ListenTCP(at)} {
        group_.Spawn([this] {
            while (true) {
                std::shared_ptr conn = lsn_->Accept();
                group_.Spawn([conn = std::move(conn)] {
                    cactus::BufferedReader{conn.get()}.WriteTo(conn.get());
                });
            }
        });
    }

    const cactus::SocketAddress& GetAddress() const {
        return lsn_->Address();
    }

private:
    std::unique_ptr<cactus::IListener> lsn_;
    cactus::ServerGroup group_;
};

template <bool IsOk = true>
std::unique_ptr<cactus::IConn> MakeConnection(const cactus::SocketAddress& address,
                                              std::string_view request = kFakeRequest) {
    auto conn = cactus::DialTCP(address);
    conn->Write(cactus::View(request));
    char reply[8];
    conn->ReadFull(cactus::View(reply));
    if constexpr (IsOk) {
        REQUIRE(std::ranges::equal(reply, kOkReply));
    } else {
        REQUIRE(std::ranges::equal(reply, kErrorReply));
    }
    return conn;
}

std::unique_ptr<cactus::IConn> MakeConnectionSlowly(const cactus::SocketAddress& address) {
    auto conn = cactus::DialTCP(address);
    for (auto c : kFakeRequest) {
        conn->Write(cactus::View(c));
        cactus::SleepFor(50ms);
    }
    char reply[8];
    conn->ReadFull(cactus::View(reply));
    REQUIRE(std::ranges::equal(reply, kOkReply));
    return conn;
}

void RunEchoClients(const cactus::SocketAddress& address, size_t count) {
    constexpr auto kNumIters = 10;
    constexpr auto kSleepTime = 100ms;
    constexpr auto kTotalTime = kNumIters * kSleepTime;

    cactus::WaitGroup group;
    for (size_t i = 0; i < count; ++i) {
        group.Spawn([&, i] {
            std::mt19937_64 gen{i};
            std::vector<uint64_t> result(1'000);
            std::vector<uint64_t> expected(1'000);
            auto conn = MakeConnection(address);
            for (auto j = 0; j < kNumIters; ++j) {
                std::ranges::generate(expected, gen);
                conn->Write(cactus::View(expected));
                cactus::SleepFor(kSleepTime);
                conn->ReadFull(cactus::View(result));
                CHECK(result == expected);
            }
            --count;
        });
    }

    CPUTimer timer;
    group.Wait();

    CHECK(count == 0);
    auto [wall_time, cpu_time] = timer.GetTimes();
    CHECK(wall_time < 1.5 * kTotalTime);
    CHECK(cpu_time < 0.5 * kTotalTime);
}

class SocksTest {
public:
    SocksTest() {
        group_.Spawn([&] { socks_.Serve(); });
    }

    const cactus::SocketAddress& GetAddress() const {
        return socks_.GetAddress();
    }

private:
    SocksServer socks_{{"127.0.0.1", 0}};
    EchoServer echo_{kServerAddress};
    cactus::ServerGroup group_;
};

}  // namespace

FIBER_TEST_CASE("EchoProxy") {
    SocksTest socks;

    SECTION("1 client") {
        RunEchoClients(socks.GetAddress(), 1);
    }

    SECTION("10 clients") {
        RunEchoClients(socks.GetAddress(), 10);
    }

    SECTION("100 clients") {
        RunEchoClients(socks.GetAddress(), 100);
    }
}

FIBER_TEST_CASE("SlowConnection") {
    SocksTest socks;
    auto conn = MakeConnectionSlowly(socks.GetAddress());
    for (auto i : std::views::iota(0, 10)) {
        auto view = cactus::View(i);
        conn->Write(view.subspan(0, 2));
        cactus::SleepFor(10ms);
        conn->Write(view.subspan(2));
        auto x = -1;
        conn->ReadFull(cactus::View(x));
        CHECK(x == i);
    }
}

FIBER_TEST_CASE("UserIdLength") {
    SocksTest socks;
    std::string prefix{kFakeRequest.substr(0, 8)};

    for (auto length : {0, 10, 100, 256}) {
        auto request = prefix + std::string(length, 'a') + '\0';
        MakeConnection(socks.GetAddress(), request);
    }

    for (auto length : {257, 500, 1'000}) {
        auto request = prefix + std::string(length, 'a') + '\0';
        MakeConnection<false>(socks.GetAddress(), request);
    }
}

FIBER_TEST_CASE("CloseBeforeFullRequest") {
    SocksTest socks;
    for (size_t i = 0; i < kFakeRequest.size(); ++i) {
        auto conn = cactus::DialTCP(socks.GetAddress());
        conn->Write(cactus::View(kFakeRequest.substr(0, i)));
        conn->CloseWrite();
        CHECK(conn->Read(cactus::View(42)) == 0);
    }
    MakeConnection(socks.GetAddress());
}

FIBER_TEST_CASE("InvalidRequest") {
    SocksTest socks;

    SECTION("VN") {
        std::string s{kFakeRequest};
        s[0] = 5;
        MakeConnection<false>(socks.GetAddress(), s);
    }

    SECTION("CD") {
        std::string s{kFakeRequest};
        s[1] = 9;
        MakeConnection<false>(socks.GetAddress(), s);
    }
}

FIBER_TEST_CASE("NoServer") {
    SocksServer socks{{"127.0.0.1", 0}};
    cactus::ServerGroup group;
    group.Spawn([&] { socks.Serve(); });
    MakeConnection<false>(socks.GetAddress());
}

FIBER_TEST_CASE("HalfOpen") {
    SocksServer socks{{"127.0.0.1", 0}};
    auto lsn = cactus::ListenTCP(kServerAddress);

    cactus::ServerGroup group;
    group.Spawn([&] { socks.Serve(); });

    SECTION("Client") {
        group.Spawn([&] {
            auto conn = lsn->Accept();
            auto x = -1;
            conn->ReadFull(cactus::View(x));
            CHECK(x == 42);
            CHECK(conn->Read(cactus::View(x)) == 0);
            conn->Write(cactus::View(10));
        });

        auto conn = MakeConnection(socks.GetAddress());
        auto x = 42;
        conn->Write(cactus::View(x));
        conn->CloseWrite();
        conn->ReadFull(cactus::View(x));
        CHECK(x == 10);
    }

    SECTION("Server") {
        group.Spawn([&] {
            auto conn = MakeConnection(socks.GetAddress());
            auto x = -1;
            conn->ReadFull(cactus::View(x));
            CHECK(x == 42);
            CHECK(conn->Read(cactus::View(x)) == 0);
            conn->Write(cactus::View(10));
        });

        auto conn = lsn->Accept();
        auto x = 42;
        conn->Write(cactus::View(x));
        conn->CloseWrite();
        conn->ReadFull(cactus::View(x));
        CHECK(x == 10);
    }
}
