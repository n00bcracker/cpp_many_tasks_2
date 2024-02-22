#include "commons.h"

#include <algorithm>
#include <unordered_set>
#include <random>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

using Catch::Generators::chunk;
using Catch::Generators::random;

void CheckSubsets(const std::vector<int64_t>& data, const std::vector<size_t>& first_indices,
                  const std::vector<size_t>& second_indices) {
    REQUIRE_FALSE(first_indices.empty());
    REQUIRE_FALSE(second_indices.empty());

    std::unordered_set<size_t> used_indices;
    int64_t sum = 0;
    for (auto i : first_indices) {
        sum += data.at(i);
        REQUIRE(used_indices.insert(i).second);
    }
    for (auto i : second_indices) {
        sum -= data.at(i);
        REQUIRE(used_indices.insert(i).second);
    }
    REQUIRE(sum == 0);
}

std::vector<int64_t> GenerateFalse(size_t size) {
    auto bit = 1u << size;
    auto m = random(bit, 2 * bit - 1).get();
    auto data = chunk(size, random(-10l, 10l)).get();
    for (auto& x : data) {
        bit >>= 1;
        x = x * m + bit;
    }
    return data;
}

std::vector<int64_t> GenerateTrue(size_t size) {
    static constexpr auto kValue = std::numeric_limits<int64_t>::max() / 1'000;
    static std::mt19937 gen{73};
    auto num_elements = random(2ul, size).get();
    auto first_size = random(1ul, num_elements - 1).get();
    auto data = chunk(size, random(-kValue, kValue)).get();
    auto sum = std::reduce(data.begin(), data.begin() + first_size);
    sum -= std::reduce(data.begin() + first_size, data.begin() + num_elements);
    data[0] -= sum;
    std::ranges::shuffle(data, gen);
    return data;
}
