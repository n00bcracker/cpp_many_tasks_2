#include "../hazard-ptr/hazard_ptr.h"
#include "sync_map.h"

#include <random>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Simple") {
    RegisterThread();
    SyncMap<int, int> map;

    int value;
    REQUIRE_FALSE(map.Lookup(0, &value));

    REQUIRE(map.Insert(0, 42));
    REQUIRE(map.Lookup(0, &value));
    REQUIRE(value == 42);

    REQUIRE_FALSE(map.Insert(0, 42));
    UnregisterThread();
}

TEST_CASE("UpgradeToReadOnly") {
    RegisterThread();
    SyncMap<int, int> map;

    map.Insert(0, 42);
    map.Insert(1, 42);
    map.Insert(2, 42);

    for (auto i = 0; i < 1024; ++i) {
        int value{};
        REQUIRE(map.Lookup(0, &value));
        REQUIRE(value == 42);
    }
    UnregisterThread();
}

TEST_CASE("Stress") {
    std::unordered_map<int, int> std_map;
    SyncMap<int, int> map;

    std::mt19937_64 gen{42};
    std::uniform_int_distribution dist{0, 100'000};

    for (auto i = 0; i < 1'000'000; ++i) {
        auto key = dist(gen);
        int value;

        auto it = std_map.find(key);
        auto found = map.Lookup(key, &value);
        if (it == std_map.end()) {
            REQUIRE_FALSE(found);
        } else {
            REQUIRE(found);
            REQUIRE(value == it->second);
        }

        if (!found && (dist(gen) % 2)) {
            value = dist(gen);
            map.Insert(key, value);
            std_map[key] = value;
        }
    }
}
