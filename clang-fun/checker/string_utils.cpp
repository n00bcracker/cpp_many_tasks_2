#include "string_utils.h"

#include <cctype>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>
#include <algorithm>

unsigned LevenshteinDistance(const std::string_view& first_str,
                             const std::string_view& second_str) {
    size_t first_len = first_str.size();
    size_t second_len = second_str.size();

    std::vector<unsigned> prew_row(first_len + 1, 0);
    std::vector<unsigned> current_row(first_len + 1, 0);

    for (size_t i = 0; i < first_len + 1; ++i) {
        prew_row[i] = i;
    }

    for (size_t j = 0; j < second_len; ++j) {
        current_row[0] = j + 1;

        for (size_t i = 1; i < first_len + 1; ++i) {
            unsigned substitution_cost;
            if (std::tolower(second_str[j]) == std::tolower(first_str[i - 1])) {
                substitution_cost = prew_row[i - 1];
            } else {
                substitution_cost = prew_row[i - 1] + 1;
            }

            current_row[i] = std::min({substitution_cost, prew_row[i] + 1, current_row[i - 1] + 1});
        }
        std::swap(current_row, prew_row);
    }

    return prew_row.back();
}

std::vector<std::string_view> SplitIntoWords(const std::string& name) {
    std::vector<std::string_view> res;
    if (name.find('_') != std::string::npos) {
        size_t cur_pos = 0;
        size_t found_pos;
        while ((found_pos = name.find('_', cur_pos)) != std::string::npos) {
            res.emplace_back(std::string_view(name.c_str() + cur_pos, found_pos - cur_pos));
            cur_pos = found_pos + 1;
        }

        if (cur_pos < name.size()) {
            res.emplace_back(std::string_view(name.c_str() + cur_pos, name.size() - cur_pos));
        }
    } else {
        size_t start_pos = 0;
        size_t cur_pos = 0;
        while (cur_pos < name.size()) {
            if (name[cur_pos] >= 'A' && name[cur_pos] <= 'Z') {
                if (cur_pos > start_pos) {
                    res.emplace_back(
                        std::string_view(name.c_str() + start_pos, cur_pos - start_pos));
                }
                start_pos = cur_pos;
            }

            ++cur_pos;
        }

        res.emplace_back(std::string_view(name.c_str() + start_pos, cur_pos - start_pos));

        std::vector<std::string_view> tmp(std::move(res));
        const char* word_start = nullptr;
        size_t word_size = 0;
        for (auto& word : tmp) {
            if (word.size() == 1 && (word[0] >= 'A' && word[0] <= 'Z')) {
                if (!word_start) {
                    word_start = word.data();
                }

                ++word_size;
            } else {
                if (word_start) {
                    res.emplace_back(std::string_view(word_start, word_size));
                    word_start = nullptr;
                    word_size = 0;
                }

                res.emplace_back(word);
            }
        }

        if (word_start) {
            res.emplace_back(std::string_view(word_start, word_size));
        }
    }

    return res;
}
