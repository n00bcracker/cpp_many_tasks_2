#include "find_subsets.h"
#include "commons.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

TEST_CASE("Benchmark") {
    constexpr auto kSize = 30u;
    Subsets subsets{};

    auto data = GenerateFalse(kSize);
    BENCHMARK("False") {
        subsets = FindEqualSumSubsets(data);
    };
    REQUIRE_FALSE(subsets.exists);

    data = GenerateTrue(kSize);
    BENCHMARK("True") {
        subsets = FindEqualSumSubsets(data);
    };
    REQUIRE(subsets.exists);
    CheckSubsets(data, subsets.first_indices, subsets.second_indices);

    data.clear();
    for (auto x = (1 << (kSize - 1)); x; x >>= 1) {
        data.push_back(x);
    }
    --data.front();
    BENCHMARK("Powers of two") {
        subsets = FindEqualSumSubsets(data);
    };
    REQUIRE(subsets.exists);
    CheckSubsets(data, subsets.first_indices, subsets.second_indices);
}
