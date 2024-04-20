#include "simple_storage.h"
#include "server.h"
#include "util.h"

#include <utility>
#include <vector>
#include <string>
#include <string_view>
#include <chrono>

#include <cactus/cactus.h>
#include <cactus/test.h>

#include <catch2/generators/catch_generators.hpp>

using namespace std::chrono_literals;

namespace {

using Script = std::vector<std::pair<std::string_view, std::string_view>>;

void Test(const Script& script) {
    auto storage = std::make_unique<redis::SimpleStorage>();
    redis::Server server{{"127.0.0.1", 0}, std::move(storage)};
    server.Run();

    auto conn = cactus::DialTCP(server.GetAddress());
    std::string rsp;

    for (const auto& [req, expected_rsp] : script) {
        for (auto c : req) {
            conn->Write(cactus::View(c));
            cactus::Yield();
        }

        rsp.resize(expected_rsp.size());
        {
            cactus::TimeoutGuard guard{1s};
            conn->ReadFull(cactus::View(rsp));
        }
        REQUIRE(rsp == expected_rsp);
    }
}

}  // namespace

TEST_CASE("StorageGetSet") {
    redis::SimpleStorage storage;

    REQUIRE_FALSE(storage.Get("a").has_value());

    storage.Set("a", "foo");
    REQUIRE(storage.Get("a") == "foo");

    REQUIRE_FALSE(storage.Get("b").has_value());
    storage.Set("b", "baz");
    REQUIRE(storage.Get("b") == "baz");

    storage.Set("a", "bar");
    CHECK(storage.Get("a") == "bar");
    CHECK(storage.Get("b") == "baz");
}

TEST_CASE("StorageDelta") {
    redis::SimpleStorage storage;

    storage.Set("a", "15");
    REQUIRE(storage.Change("a", 5) == 20);
    REQUIRE(storage.Change("a", -10) == 10);
    CHECK(storage.Get("a") == "10");

    storage.Set("b", "-5");
    REQUIRE(storage.Change("b", -12) == -17);
    REQUIRE(storage.Change("b", 15) == -2);
    REQUIRE(storage.Change("b", 0) == -2);
    CHECK(storage.Get("b") == "-2");

    CHECK(storage.Change("c", 1) == 1);
    CHECK(storage.Change("d", -1) == -1);
}

TEST_CASE("StorageNaN") {
    redis::SimpleStorage storage;

    storage.Set("a", "aba");
    CHECK_THROWS(storage.Change("a", 4));

    storage.Set("b", "-1caba");
    CHECK_THROWS(storage.Change("b", 5));

    storage.Set("c", std::string_view{"1\0", 2});
    CHECK_THROWS(storage.Change("c", 6));
}

TEST_CASE("StorageOverflow") {
    constexpr auto kMin = std::numeric_limits<int64_t>::min();
    constexpr auto kMax = std::numeric_limits<int64_t>::max();

    redis::SimpleStorage storage;

    SECTION("Min") {
        storage.Set("x", std::to_string(kMin));
        REQUIRE_THROWS(storage.Change("x", -1));
        REQUIRE_THROWS(storage.Change("x", -4));
        REQUIRE_THROWS(storage.Change("x", kMin));
        REQUIRE(storage.Change("x", kMax) == -1);

        storage.Set("x", "0");
        REQUIRE(storage.Change("x", kMin) == kMin);
        storage.Set("x", "-6");
        REQUIRE(storage.Change("x", kMin + 6) == kMin);
        storage.Set("x", "-7");
        REQUIRE_THROWS(storage.Change("x", kMin + 6));
    }

    SECTION("Max") {
        storage.Set("x", std::to_string(kMax));
        REQUIRE_THROWS(storage.Change("x", 1));
        REQUIRE_THROWS(storage.Change("x", 3));
        REQUIRE_THROWS(storage.Change("x", kMax));
        REQUIRE(storage.Change("x", kMin) == -1);

        storage.Set("x", "0");
        REQUIRE(storage.Change("x", kMax) == kMax);
        storage.Set("x", "12");
        REQUIRE(storage.Change("x", kMax - 12) == kMax);
        storage.Set("x", "13");
        REQUIRE_THROWS(storage.Change("x", kMax - 12));
    }

    SECTION("Positive") {
        storage.Set("x", std::string(100, '1'));
        REQUIRE_THROWS(storage.Change("x", 1));
        REQUIRE_THROWS(storage.Change("x", -1));
    }

    SECTION("Negative") {
        storage.Set("x", '-' + std::string(200, '2'));
        REQUIRE_THROWS(storage.Change("x", 1));
        REQUIRE_THROWS(storage.Change("x", -1));
    }
}

FIBER_TEST_CASE("GetSet") {
    Test({{"*3\r\n"
           "$3\r\nSET\r\n"
           "$3\r\nkey\r\n"
           "$5\r\nvalue\r\n",
           "$2\r\nOK\r\n"},
          {"*2\r\n"
           "$3\r\nGET\r\n"
           "$3\r\nkey\r\n",
           "$5\r\nvalue\r\n"},
          {"*2\r\n"
           "$3\r\nGET\r\n"
           "$7\r\nnot-key\r\n",
           "$-1\r\n"},
          {"*3\r\n"
           "$3\r\nSET\r\n"
           "$3\r\nkey\r\n"
           "$7\r\nanother\r\n",
           "$2\r\nOK\r\n"},
          {"*2\r\n"
           "$3\r\nGET\r\n"
           "$3\r\nkey\r\n",
           "$7\r\nanother\r\n"}});
}

FIBER_TEST_CASE("IncrementDecrement") {
    Test({{"*2\r\n"
           "$4\r\nINCR\r\n"
           "$3\r\nkey\r\n",
           ":1\r\n"},
          {"*2\r\n"
           "$4\r\nINCR\r\n"
           "$3\r\nkey\r\n",
           ":2\r\n"},
          {"*2\r\n"
           "$4\r\nDECR\r\n"
           "$3\r\nkey\r\n",
           ":1\r\n"},
          {"*2\r\n"
           "$4\r\nDECR\r\n"
           "$3\r\nkey\r\n",
           ":0\r\n"},
          {"*2\r\n"
           "$4\r\nDECR\r\n"
           "$3\r\nkey\r\n",
           ":-1\r\n"}});
}

FIBER_TEST_CASE("IncrementAfterSet") {
    Test({{"*3\r\n"
           "$3\r\nSET\r\n"
           "$4\r\ndoor\r\n"
           "$2\r\n-3\r\n",
           "$2\r\nOK\r\n"},
          {"*2\r\n"
           "$4\r\nINCR\r\n"
           "$4\r\ndoor\r\n",
           ":-2\r\n"},
          {"*2\r\n"
           "$4\r\nINCR\r\n"
           "$4\r\ndoor\r\n",
           ":-1\r\n"},
          {"*2\r\n"
           "$3\r\nGET\r\n"
           "$4\r\ndoor\r\n",
           "$2\r\n-1\r\n"}});
}

FIBER_TEST_CASE("ManyClients") {
    auto storage = std::make_unique<redis::SimpleStorage>();
    redis::Server server{{"127.0.0.1", 0}, std::move(storage)};
    server.Run();

    cactus::WaitGroup group;
    for (auto i = 0; i < 10; ++i) {
        group.Spawn([&server, i] {
            CPUTimer timer;
            auto conn = cactus::DialTCP(server.GetAddress());

            std::string request =
                "*3\r\n"
                "$3\r\nSET\r\n"
                "$1\r\n" +
                std::to_string(i) +
                "\r\n"
                "$1\r\n" +
                std::to_string(9 - i) + "\r\n";
            for (auto c : request) {
                conn->Write(cactus::View(c));
                cactus::SleepFor(10ms);
            }

            std::string expected_rsp = "$2\r\nOK\r\n";
            std::string rsp;
            rsp.resize(expected_rsp.size());
            conn->ReadFull(cactus::View(rsp));
            CHECK(rsp == expected_rsp);

            auto wall_time = timer.GetTimes().wall_time;
            CHECK(wall_time < 20ms * request.size());
        });
    }
    group.Wait();
}

FIBER_TEST_CASE("WrongRequest") {
    auto storage = std::make_unique<redis::SimpleStorage>();
    redis::Server server{{"127.0.0.1", 0}, std::move(storage)};
    server.Run();

    auto conn = cactus::DialTCP(server.GetAddress());
    std::string_view req =
        GENERATE(":3\r\n", "*2\r\n$3\r\nFOO\r\n$2\r\nab\r\n", "*2\r\n$3\r\nGET\r\n$2\r\nabc\r\n");
    conn->Write(cactus::View(req));

    char buffer;
    CHECK_THROWS(conn->ReadFull(cactus::View(buffer)));
}
