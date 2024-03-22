#include "fanout.h"

#include <chrono>
#include <numeric>
#include <string>

#include <cactus/test.h>

using namespace std::chrono_literals;

template <uint64_t N>
void Test(const uint64_t (&array)[N], const uint64_t (&secrets)[N]) {
    static_assert(N > 0);
    static constexpr uint16_t kPort = 13'500;
    static const cactus::SocketAddress kAddress{"127.0.0.1", kPort};

    cactus::ServerGroup group;
    for (auto i = 0ul; i < N; ++i) {
        group.Spawn([&, i] {
            auto port = static_cast<uint16_t>(kPort + i + 1);
            auto lsn = cactus::ListenTCP({kAddress.GetIp(), port});
            auto conn = lsn->Accept();
            uint64_t data;
            conn->ReadFull(cactus::View(data));
            cactus::SleepFor(500ms);
            if (data == array[i]) {
                conn->Write(cactus::View(secrets[i]));
            }
        });
    }

    auto sum = std::reduce(std::begin(secrets), std::end(secrets), 0ul);
    auto flag = "sum is " + std::to_string(sum);
    cactus::Fiber master{[&] {
        auto lsn = cactus::ListenTCP(kAddress);
        auto conn = lsn->Accept();
        conn->Write(cactus::View(N));
        conn->Write(cactus::View(array));
        uint64_t answer;
        conn->ReadFull(cactus::View(answer));
        cactus::SleepFor(250ms);
        if (answer == sum) {
            conn->Write(cactus::View(flag));
        }
    }};

    cactus::Yield();
    auto start = std::chrono::steady_clock::now();
    CHECK(Fanout(kAddress) == flag);
    auto duration = std::chrono::steady_clock::now() - start;
    CHECK(duration < 1s);
}

FIBER_TEST_CASE("Test fanout") {
    Test({10}, {15});
    Test({1, 2, 3}, {10, 15, 20});
    Test({54, 23, 123, 234, 135}, {12, 64, 65, 34, 1});
}
