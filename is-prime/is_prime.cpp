#include "is_prime.h"

#include <cmath>
#include <algorithm>
#include <cstddef>
#include <vector>
#include <atomic>
#include <thread>

constexpr size_t kThreadCnt = 16;
std::atomic_flag is_not_prime;

void ThreadFunc(uint64_t x, uint64_t l_bound, uint64_t r_bound) {
    for (size_t i = l_bound; i < r_bound; ++i) {
        if (!is_not_prime.test()) {
            if (x % i == 0) {
                is_not_prime.test_and_set();
                break;
            }
        } else {
            break;
        }
    }
}

bool IsPrime(uint64_t x) {
    if (x < 2) {
        return false;
    }
    auto root = static_cast<uint64_t>(std::sqrt(x));
    auto bound = std::min(root + 6, x);

    is_not_prime.clear();
    if ((bound - 2) / kThreadCnt < 10) {
        ThreadFunc(x, 2, bound);
    } else {
        std::vector<std::thread> threads;
        uint64_t l_bound = 2;
        uint64_t std_interval_len = (bound - 2) / kThreadCnt;
        uint64_t full_threads_cnt = (bound - 2) % kThreadCnt;
        for (size_t i = 0; i < kThreadCnt; ++i) {
            auto interval_len = i < full_threads_cnt ? std_interval_len + 1 : std_interval_len;
            threads.emplace_back(ThreadFunc, x, l_bound, l_bound + interval_len);
            l_bound += interval_len;
        }

        for (auto& thread : threads) {
            thread.join();
        }
    }

    return !is_not_prime.test();
}
