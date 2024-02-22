#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

void CheckSubsets(const std::vector<int64_t>& data, const std::vector<size_t>& first_indices,
                  const std::vector<size_t>& second_indices);
std::vector<int64_t> GenerateFalse(size_t size);
std::vector<int64_t> GenerateTrue(size_t size);
