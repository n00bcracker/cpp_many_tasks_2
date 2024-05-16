#pragma once

#include <string>
#include <string_view>
#include <vector>

unsigned LevenshteinDistance(const std::string_view& first_str, const std::string_view& second_str);

std::vector<std::string_view> SplitIntoWords(const std::string& name);