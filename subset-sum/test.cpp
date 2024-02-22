#include "find_subsets.h"
#include "commons.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

using Catch::Generators::random;
using Catch::Generators::take;

static void Test(const std::vector<int64_t>& data, bool answer_exists) {
    auto subsets = FindEqualSumSubsets(data);
    REQUIRE(answer_exists == subsets.exists);
    if (answer_exists) {
        CheckSubsets(data, subsets.first_indices, subsets.second_indices);
    }
}

TEST_CASE("Simple") {
    Test({1, 2, 4, 8, 15}, true);
}

TEST_CASE("Empty") {
    Test({}, false);
}

TEST_CASE("EmptyPart") {
    Test({5, -5}, false);
}

TEST_CASE("Single") {
    Test({1}, false);
}

TEST_CASE("EqualElems") {
    Test({2, 2, 3}, true);
}

TEST_CASE("Middle") {
    Test({1, 3, 5, 12, 20, -1}, true);
}

TEST_CASE("Negative") {
    Test({1, -1, 3, -3, 5, -5, 7, -7, 15, -15, 62}, true);
}

TEST_CASE("RandomFalse") {
    auto size = GENERATE(take(1'000, random(0, 15)));
    Test(GenerateFalse(size), false);
}

TEST_CASE("RandomTrue") {
    auto size = GENERATE(take(1'000, random(2, 15)));
    Test(GenerateTrue(size), true);
}
