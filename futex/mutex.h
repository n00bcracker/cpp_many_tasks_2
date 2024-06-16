#pragma once

#include <atomic>
#include <cstdint>

#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>

// Atomically do the following:
//    if (*value == expected_value) {
//        sleep_on_address(value)
//    }
inline void FutexWait(int* value, int expected_value) {
    syscall(SYS_futex, value, FUTEX_WAIT_PRIVATE, expected_value, nullptr, nullptr, 0);
}

// Wakeup 'count' threads sleeping on address of value (-1 wakes all)
inline void FutexWake(int* value, int count) {
    syscall(SYS_futex, value, FUTEX_WAKE_PRIVATE, count, nullptr, nullptr, 0);
}

class Mutex {
public:
    Mutex() : state_(0), state_ref_(state_) {
    }

    void Lock() {
        int32_t state = 0;
        if (!state_ref_.compare_exchange_strong(state, 1, std::memory_order_acq_rel)) {
            if (state != 2) {
                state = state_ref_.exchange(2, std::memory_order_acq_rel);
            }

            while (state != 0) {
                FutexWait(&state_, 2);
                state = state_ref_.exchange(2, std::memory_order_acq_rel);
            }
        }
    }

    void Unlock() {
        if (state_ref_.fetch_sub(1, std::memory_order_acq_rel) != 1) {
            state_ref_ = 0;
            FutexWake(&state_, 1);
        }
    }

private:
    alignas(std::atomic_ref<int>::required_alignment) int state_;
    std::atomic_ref<int> state_ref_;
};
