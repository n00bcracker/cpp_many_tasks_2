#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>

template <class T>
class UnbufferedChannel {
public:
    void Send(const T& value) {
        std::unique_lock<std::mutex> write(write_);
        may_write_.wait(write,
                        [this] { return !is_filled_.test() || is_closed_.test(); });
        if (is_closed_.test()) {
            throw std::runtime_error("Channel is closed");
        }

        uint64_t my_num = ++writers_counter_;
        value_ = value;
        is_filled_.test_and_set();
        may_read_.notify_one();
        write.unlock();

        std::unique_lock<std::mutex> exit(write_);
        may_exit_.wait(exit, [this, &my_num] { return (my_num <= readers_counter_) || is_closed_.test(); });
        if (my_num > readers_counter_) {
            throw std::runtime_error("Channel is closed");
        }
        may_write_.notify_one();
    }

    void Send(T&& value) {
        std::unique_lock<std::mutex> write(write_);
        may_write_.wait(write,
                        [this] { return !is_filled_.test() || is_closed_.test(); });
        if (is_closed_.test()) {
            throw std::runtime_error("Channel is closed");
        }

        uint64_t my_num = ++writers_counter_;
        value_ = std::forward<T>(value);
        is_filled_.test_and_set();
        may_read_.notify_one();
        write.unlock();

        std::unique_lock<std::mutex> exit(write_);
        may_exit_.wait(exit, [this, &my_num] { return (my_num <= readers_counter_) || is_closed_.test(); });
        if (my_num > readers_counter_) {
            throw std::runtime_error("Channel is closed");
        }
        may_write_.notify_one();
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> read(write_);
        may_read_.wait(read, [this] { return is_filled_.test() || is_closed_.test(); });

        if (is_closed_.test()) {
            return std::nullopt;
        } else {
            T res = std::move(value_);
            ++readers_counter_;
            is_filled_.clear();
            may_exit_.notify_one();
            return std::optional<T>(std::move(res));
        }
    }

    void Close() {
        std::unique_lock<std::mutex> close(write_);
        is_closed_.test_and_set();
        may_exit_.notify_all();
        may_read_.notify_all();
        may_write_.notify_all();
    }

private:
    // std::mutex read_;
    std::mutex write_;
    // std::mutex exit_;
    std::condition_variable may_write_;
    std::condition_variable may_read_;
    std::condition_variable may_exit_;
    std::atomic_flag is_filled_;
    std::atomic_flag is_closed_;
    std::atomic_uint64_t readers_counter_ = 0;
    std::atomic_uint64_t writers_counter_ = 0;
    T value_;
};
