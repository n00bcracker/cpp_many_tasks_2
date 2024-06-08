#pragma once

#include <atomic>
#include <thread>

class SpinLock {
public:
    void Lock() {
        while (atomic_lock_.test_and_set()) {
            std::this_thread::yield();
        }
    }

    void Unlock() {
        atomic_lock_.clear();
    }

private:
    std::atomic_flag atomic_lock_;
};
