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
                        [this] { return state_ == 0 || is_closed_.test(); });
        if (is_closed_.test()) {
            throw std::runtime_error("Channel is closed");
        }

        value_ = value;
        state_ = 1;
        may_read_.notify_one();
        write.unlock();

        std::unique_lock<std::mutex> exit(exit_);
        may_exit_.wait(exit, [this] { return state_ == 2 || is_closed_.test(); });
        if (state_ != 2) {
            throw std::runtime_error("Channel is closed");
        }
        state_ = 0;
        may_write_.notify_one();
    }

    void Send(T&& value) {
        std::unique_lock<std::mutex> write(write_);
        may_write_.wait(write,
                        [this] { return state_ == 0 || is_closed_.test(); });
        if (is_closed_.test()) {
            throw std::runtime_error("Channel is closed");
        }

        value_ = std::forward<T>(value);
        state_ = 1;
        may_read_.notify_one();
        write.unlock();

        std::unique_lock<std::mutex> exit(exit_);
        may_exit_.wait(exit, [this] { return state_ == 2 || is_closed_.test(); });
        if (state_ != 2) {
            throw std::runtime_error("Channel is closed");
        }
        state_ = 0;
        may_write_.notify_one();
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> read(read_);
        may_read_.wait(read, [this] { return state_ == 1 || is_closed_.test(); });

        if (is_closed_.test()) {
            return std::nullopt;
        } else {
            T res = std::move(value_);
            state_ = 2;
            may_exit_.notify_one();
            return std::optional<T>(std::move(res));
        }
    }

    void Close() {
        std::unique_lock<std::mutex> close(read_);
        is_closed_.test_and_set();
        may_exit_.notify_one();
        may_read_.notify_all();
        may_write_.notify_all();
    }

private:
    std::mutex read_;
    std::mutex write_;
    std::mutex exit_;
    std::condition_variable may_write_;
    std::condition_variable may_read_;
    std::condition_variable may_exit_;
    // std::atomic_flag is_filled_;
    std::atomic_flag is_closed_;
    std::atomic_uint64_t state_ = 0;
    T value_;
};
