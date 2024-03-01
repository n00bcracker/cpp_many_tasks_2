#pragma once

#include <mutex>

class RWLock {
public:
    void Read(auto func) {
        read_.lock();
        if (++blocked_readers_ == 1) {
            global_.lock();
        }
        read_.unlock();
        try {
            func();
        } catch (...) {
            EndRead();
            throw;
        }
        EndRead();
    }

    void Write(auto func) {
        std::lock_guard lock{global_};
        func();
    }

private:
    void EndRead() {
        std::lock_guard lock{read_};
        if (--blocked_readers_ == 0) {
            global_.unlock();
        }
    }

    std::mutex read_;
    std::mutex global_;
    int blocked_readers_ = 0;
};
