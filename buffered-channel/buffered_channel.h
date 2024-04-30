#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

template <class T>
class BufferedChannel {
public:
    explicit BufferedChannel(uint32_t size) : max_size_(size) {
    }

    void Send(const T& value) {
        std::unique_lock<std::mutex> send(edit_queue_);
        may_write_.wait(send, [this] { return (queue_.size() < max_size_) || is_closed_.test(); });

        if (is_closed_.test()) {
            throw std::runtime_error("Channel is closed");
        } else {
            queue_.push(value);
            may_read_.notify_one();
        }
    }

    void Send(T&& value) {
        std::unique_lock<std::mutex> send(edit_queue_);
        may_write_.wait(send, [this] { return (queue_.size() < max_size_) || is_closed_.test(); });

        if (is_closed_.test()) {
            throw std::runtime_error("Channel is closed");
        } else {
            queue_.emplace(std::forward<T>(value));
            may_read_.notify_one();
        }
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> read(edit_queue_);
        may_read_.wait(read, [this] { return !queue_.empty() || is_closed_.test(); });

        if (!queue_.empty()) {
            T res = std::move(queue_.front());
            queue_.pop();
            may_write_.notify_one();
            return std::optional<T>(std::move(res));
        } else {
            return std::nullopt;
        }
    }

    void Close() {
        is_closed_.test_and_set();
        may_read_.notify_all();
        may_write_.notify_all();
    }

private:
    size_t max_size_;
    std::mutex edit_queue_;
    std::condition_variable may_read_;
    std::condition_variable may_write_;
    std::atomic_flag is_closed_;

    std::queue<T> queue_;
};
