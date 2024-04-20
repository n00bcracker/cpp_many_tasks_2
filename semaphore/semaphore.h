#pragma once

#include <cstddef>
#include <mutex>
#include <condition_variable>

class Semaphore {
public:
    explicit Semaphore(int count) : count_{count} {
    }

    void Acquire(auto callback) {
        std::unique_lock lock{mutex_};
        size_t query_pos = jobs_cnt_++;
        cv_.wait(lock, [&] { return count_ && query_pos == executing_pos_; });
        callback(count_);
        ++executing_pos_;
    }

    void Acquire() {
        Acquire([](int& value) { --value; });
    }

    void Release() {
        std::lock_guard lock{mutex_};
        ++count_;
        cv_.notify_all();
    }

private:
    int count_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t executing_pos_ = 0;
    size_t jobs_cnt_ = 0;
};
