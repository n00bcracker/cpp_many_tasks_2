#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>

template <class T>
class UnbufferedChannel {
public:
    void Send(const T& value) {
        std::unique_lock<std::mutex> write(edit_value_);
        may_write_.wait(write,
                        [this] { return (!is_filled_ && !has_waiter_) || is_closed_.test(); });
        if (is_closed_.test()) {
            throw std::runtime_error("Channel is closed");
        }

        value_ = value;
        is_filled_ = true;
        may_read_.notify_one();
        has_waiter_ = true;
        may_exit_.wait(write, [this] { return !is_filled_ || is_closed_.test(); });
        has_waiter_ = false;
        if (is_filled_) {
            throw std::runtime_error("Channel is closed");
        }
        may_write_.notify_one();
    }

    void Send(T&& value) {
        std::unique_lock<std::mutex> write(edit_value_);
        may_write_.wait(write,
                        [this] { return (!is_filled_ && !has_waiter_) || is_closed_.test(); });
        if (is_closed_.test()) {
            throw std::runtime_error("Channel is closed");
        }

        value_ = std::forward<T>(value);
        is_filled_ = true;
        may_read_.notify_one();
        has_waiter_ = true;
        may_exit_.wait(write, [this] { return !is_filled_ || is_closed_.test(); });
        has_waiter_ = false;
        if (is_filled_) {
            throw std::runtime_error("Channel is closed");
        }
        may_write_.notify_one();
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> read(edit_value_);
        may_read_.wait(read, [this] { return is_filled_ || is_closed_.test(); });

        if (is_closed_.test()) {
            return std::nullopt;
        } else {
            T res = std::move(value_);
            is_filled_ = false;
            may_exit_.notify_one();
            return std::optional<T>(std::move(res));
        }
    }

    void Close() {
        is_closed_.test_and_set();
        may_exit_.notify_one();
        may_read_.notify_all();
        may_write_.notify_all();
    }
private:
    std::mutex edit_value_;
    std::condition_variable may_write_;
    std::condition_variable may_read_;
    std::condition_variable may_exit_;
    bool is_filled_ = false;
    bool has_waiter_ = false;
    std::atomic_flag is_closed_;
    T value_;
};
