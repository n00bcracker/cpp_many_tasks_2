#pragma once

#include <atomic>
#include <thread>
class RWSpinLock {
public:
    void LockRead() {
        uint lock = rw_lock_.load();
        do {
            lock = (lock >> 1) << 1;
        } while (!rw_lock_.compare_exchange_strong(lock, ((lock >> 1) + 1) << 1));
    }

    void UnlockRead() {
        uint lock = rw_lock_.load();
        do {
        } while (!rw_lock_.compare_exchange_strong(lock, ((lock >> 1) - 1) << 1));
    }

    void LockWrite() {
        uint lock = rw_lock_.load();
        do {
            lock = 0;
        } while (!rw_lock_.compare_exchange_strong(lock, 1));
    }

    void UnlockWrite() {
        rw_lock_.store(0);
    }

private:
    std::atomic_uint rw_lock_;
};
