#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>

class RWLock {
public:
    void Read(auto func) {
        std::unique_lock<std::mutex> check_lock{check_cnt_};
        ++blocked_readers_cnt_;
        can_read_.wait(check_lock, [this] { return !writers_cnt_; });

        --blocked_readers_cnt_;
        ++readers_cnt_;
        check_lock.unlock();

        try {
            func();
        } catch (...) {
            EndRead();
            throw;
        }
        EndRead();
    }

    void Write(auto func) {
        std::unique_lock<std::mutex> check_lock{check_cnt_};
        ++blocked_writers_cnt_;
        can_write_.wait(check_lock, [this] { return !readers_cnt_ && !writers_cnt_; });

        --blocked_writers_cnt_;
        ++writers_cnt_;
        check_lock.unlock();

        try {
            func();
        } catch (...) {
            EndWrite();
            throw;
        }

        EndWrite();
    }

private:
    void EndRead() {
        std::lock_guard check_lock{check_cnt_};
        if (--readers_cnt_ == 0 && blocked_writers_cnt_) {
            can_write_.notify_one();
        }
    }

    void EndWrite() {
        std::lock_guard check_lock{check_cnt_};
        --writers_cnt_;
        if (blocked_readers_cnt_ > 4 * blocked_writers_cnt_) {
            can_read_.notify_all();
        } else if (blocked_writers_cnt_) {
            can_write_.notify_one();
        }
    }

    std::mutex check_cnt_;
    std::condition_variable can_read_, can_write_;
    size_t readers_cnt_ = 0;
    size_t writers_cnt_ = 0;
    size_t blocked_readers_cnt_ = 0;
    size_t blocked_writers_cnt_ = 0;
};
