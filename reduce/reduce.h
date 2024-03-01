#pragma once

#include <cstddef>
#include <iostream>
#include <numeric>
#include <iterator>
#include <thread>
#include <vector>

constexpr size_t kThreadCnt = 16;

template <std::random_access_iterator Iterator, class T>
T Reduce(Iterator first, Iterator last, const T& init, auto func) {
    size_t len = last - first;
    auto len_per_thread = len / kThreadCnt;
    auto residial = len % kThreadCnt;
    if (len_per_thread < 100) {
        return std::reduce(first, last, init, func);
    } else {
        std::vector<T> res;
        res.resize(kThreadCnt);
        std::vector<std::thread> threads;

        const auto reduce_func = [func, &first, &res](size_t start, size_t finish, size_t j) {
            auto loc = std::reduce(first + start + 1, first + finish, *(first + start), func);
            res[j] = loc;
            loc = res[j];
        };

        size_t start_pos = 0;
        for (size_t i = 0; i < kThreadCnt; ++i) {
            auto real_len_per_thread = i < residial ? len_per_thread + 1 : len_per_thread;
            threads.emplace_back(reduce_func, start_pos, start_pos + real_len_per_thread, i);
            start_pos += real_len_per_thread;
        }

        for (auto& thr : threads) {
            thr.join();
        }

        return std::reduce(res.begin(), res.end(), init, func);
    }
}
