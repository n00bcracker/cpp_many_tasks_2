#pragma once

#include <cstddef>
#include <utility>
#include "constexpr_map.h"

template <class K, class V, int S>
constexpr auto Sort(ConstexprMap<K, V, S> map) {
    auto sorted_map = map;
    const auto sort_func = [&](bool ascending) {
        if (sorted_map.Size() == 0) {
            return;
        }

        for (size_t i = 0; i < sorted_map.Size() - 1; ++i) {
            bool need_iter = false;
            for (size_t j = 0; j < sorted_map.Size() - i - 1; ++j) {
                if (ascending &&
                    sorted_map.GetByIndex(j).first > sorted_map.GetByIndex(j + 1).first) {
                    std::swap(sorted_map.GetByIndex(j), sorted_map.GetByIndex(j + 1));
                    need_iter = true;
                } else if (!ascending &&
                           sorted_map.GetByIndex(j).first < sorted_map.GetByIndex(j + 1).first) {
                    std::swap(sorted_map.GetByIndex(j), sorted_map.GetByIndex(j + 1));
                    need_iter = true;
                }
            }

            if (!need_iter) {
                break;
            }
        }
    };

    if constexpr (std::is_integral_v<K>) {
        sort_func(false);
    } else {
        sort_func(true);
    }

    return sorted_map;
}
