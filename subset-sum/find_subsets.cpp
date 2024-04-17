#include "find_subsets.h"
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

constexpr size_t kThreadCnt = 32;

struct Combination {
    size_t choice;
    size_t variant;
    bool hasTwoGroups;
};

std::pair<int64_t, bool> ComputeSum(const std::vector<int64_t>& data,
                                    const std::vector<size_t>& indices, const size_t variant) {
    int64_t res = 0;
    bool has_two_groups = false;
    size_t comb = variant;
    size_t prev_bit = comb & 1;
    for (size_t i = 0; i < indices.size(); ++i) {
        if (comb & 1) {
            res += data[indices[i]];
        } else {
            res -= data[indices[i]];
        }

        has_two_groups = has_two_groups || (prev_bit ^ (comb & 1));
        prev_bit = comb & 1;
        comb = comb >> 1;
    }

    return std::make_pair(res, has_two_groups);
}

std::vector<size_t> MakeIndicesFromChoice(const size_t choice, const size_t start_index = 0) {
    std::vector<size_t> choice_indexes;
    size_t comb = choice;
    size_t idx = 0;
    while (comb) {
        if (comb & 1) {
            choice_indexes.push_back(idx + start_index);
        }

        comb = comb >> 1;
        ++idx;
    }

    return choice_indexes;
}

std::pair<std::vector<size_t>, std::vector<size_t>> SplitIndicesIntoTwoGroups(
    const std::vector<size_t>& indices, const size_t variant) {
    size_t comb = variant;
    std::vector<size_t> first_indices, second_indices;
    for (size_t i = 0; i < indices.size(); ++i) {
        if (comb & 1) {
            first_indices.push_back(indices[i]);
        } else {
            second_indices.push_back(indices[i]);
        }

        comb = comb >> 1;
    }

    return std::make_pair(std::move(first_indices), std::move(second_indices));
}

std::pair<std::vector<size_t>, std::vector<size_t>> MakeSubsets(const Combination& comb,
                                                                const size_t start_index = 0) {
    const auto chosen_indices = MakeIndicesFromChoice(comb.choice, start_index);
    return SplitIndicesIntoTwoGroups(chosen_indices, comb.variant);
}

Subsets MergeCombinations(const Combination& first, const Combination& second,
                          const size_t start_index) {
    auto [first_indices, second_indices] = MakeSubsets(first);
    auto [indices_1, indices_2] = MakeSubsets(second, start_index);
    first_indices.insert(first_indices.end(), indices_1.begin(), indices_1.end());
    second_indices.insert(second_indices.end(), indices_2.begin(), indices_2.end());
    return {std::move(first_indices), std::move(second_indices), true};
}

void ComputeChoice(const std::vector<int64_t>& data, const size_t choice,
                   std::unordered_map<int64_t, Combination>& sums) {
    const auto chosen_indices = MakeIndicesFromChoice(choice);
    const size_t sum_variants_cnt = std::pow(2, chosen_indices.size() - 1);
    for (size_t i = 0; i < sum_variants_cnt; ++i) {
        const auto [sum, has_two_groups] = ComputeSum(data, chosen_indices, i);
        if (sums.contains(sum)) {
            if (has_two_groups && !sums[sum].hasTwoGroups) {
                sums[sum] = {choice, i, has_two_groups};
            }
        } else {
            sums.emplace(sum, Combination{choice, i, has_two_groups});
        }

        if (!sum && has_two_groups) {
            break;
        }
    }
}

std::unordered_map<int64_t, Combination> ComputeSumMap(const std::vector<int64_t>& data,
                                                       const size_t k) {
    const size_t choices_cnt = std::pow(2, k);
    std::unordered_map<int64_t, Combination> sums;
    for (size_t i = 1; i < choices_cnt; ++i) {
        ComputeChoice(data, i, sums);
        if (sums.contains(0) && sums[0].hasTwoGroups) {
            break;
        }
    }

    return sums;
}

std::optional<std::pair<Combination, Combination>> CheckChoice(
    const std::vector<int64_t>& data, const std::unordered_map<int64_t, Combination>& sums,
    const size_t choice, const size_t start_index) {
    const auto chosen_indices = MakeIndicesFromChoice(choice, start_index);
    const size_t sum_variants_cnt = std::pow(2, chosen_indices.size() - 1);
    for (size_t i = 0; i < sum_variants_cnt; ++i) {
        const auto [sum, has_two_groups] = ComputeSum(data, chosen_indices, i);
        if (sums.contains(sum)) {
            const auto& found_combination = sums.at(sum);
            if (has_two_groups || found_combination.hasTwoGroups ||
                ((found_combination.variant ^ ~i) & 1)) {
                return std::make_pair(found_combination, Combination{choice, ~i, has_two_groups});
            }
        }

        if (sums.contains(-sum)) {
            const auto& found_combination = sums.at(-sum);
            if (has_two_groups || found_combination.hasTwoGroups ||
                ((found_combination.variant ^ i) & 1)) {
                return std::make_pair(found_combination, Combination{choice, i, has_two_groups});
            }
        }

        if (!sum && has_two_groups) {
            return std::make_pair(Combination{0, 0, false}, Combination{choice, i, has_two_groups});
        }
    }

    return std::nullopt;
}

void ThreadCheckChoices(const std::vector<int64_t>& data,
                        const std::unordered_map<int64_t, Combination>& sums,
                        const size_t choice_start, const size_t choices_cnt,
                        const size_t start_index, std::atomic_flag& combination_exists,
                        std::optional<std::pair<Combination, Combination>>& found_combination) {
    for (size_t i = choice_start; i < choice_start + choices_cnt; ++i) {
        if (!combination_exists.test()) {
            auto found_combinations = CheckChoice(data, sums, i, start_index);
            if (found_combinations.has_value()) {
                if (!combination_exists.test_and_set()) {
                    found_combination = std::move(found_combinations);
                }
                break;
            }
        } else {
            break;
        }
    }
}

std::optional<std::pair<Combination, Combination>> FindSuitableCombination(
    const std::vector<int64_t>& data, const std::unordered_map<int64_t, Combination>& sums,
    const size_t k) {
    const size_t choices_cnt = std::pow(2, data.size() - k);
    const size_t choices_per_thread = (choices_cnt - 1) / kThreadCnt;
    const size_t residial = (choices_cnt - 1) % kThreadCnt;
    if (choices_per_thread < 8) {
        for (size_t i = 1; i < choices_cnt; ++i) {
            auto found_combination = CheckChoice(data, sums, i, k);
            if (found_combination.has_value()) {
                return found_combination;
            }
        }
    } else {
        auto current_choice = 1;
        std::atomic_flag combination_exists;
        std::optional<std::pair<Combination, Combination>> found_combination;
        std::vector<std::thread> threads;
        for (size_t i = 0; i < kThreadCnt; ++i) {
            const auto real_choices_per_thread =
                i < residial ? choices_per_thread + 1 : choices_per_thread;
            threads.emplace_back(ThreadCheckChoices, std::ref(data), std::ref(sums), current_choice,
                                 real_choices_per_thread, k, std::ref(combination_exists),
                                 std::ref(found_combination));
            current_choice += real_choices_per_thread;
        }

        for (auto& thread : threads) {
            thread.join();
        }

        return found_combination;
    }

    return std::nullopt;
}

Subsets FindEqualSumSubsets(const std::vector<int64_t>& data) {
    if (data.size() < 2) {
        return {};
    }

    const size_t k = data.size() > 2 ? data.size() / 2 : 1;
    const auto sums = ComputeSumMap(data, k);

    if (sums.contains(0) && sums.at(0).hasTwoGroups) {
        auto subsets = MakeSubsets(sums.at(0));
        return {std::move(subsets.first), std::move(subsets.second), true};
    }

    const auto found_combinations = FindSuitableCombination(data, sums, k);
    if (found_combinations.has_value()) {
        return MergeCombinations(found_combinations->first, found_combinations->second, k);
    } else {
        return {};
    }
}
