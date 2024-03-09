#pragma once

#include <mutex>

class SpinLock {
public:
    void Lock() {
        mutex_.lock();
    }

    void Unlock() {
        mutex_.unlock();
    }

private:
    std::mutex mutex_;
};
