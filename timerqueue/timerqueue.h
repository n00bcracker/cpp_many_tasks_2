#pragma once

#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <utility>

template <class T>
class TimerQueue {
public:
    template <class... Args>
    void Add(std::chrono::system_clock::time_point at, Args&&... args) {
        std::unique_lock<std::mutex> add_elem_lock(edit_queue_);
        queue_.emplace(std::make_pair(at, T{std::forward<Args>(args)...}));

        std::thread([this, at] {
            std::this_thread::sleep_until(at);
            waiting_pop_.notify_one();
        }).detach();
    }

    T Pop() {
        std::unique_lock<std::mutex> pop_elem_lock(edit_queue_);
        waiting_pop_.wait(pop_elem_lock, [this] {
            return !queue_.empty() && std::chrono::system_clock::now() >= queue_.begin()->first;
        });

        T res(std::move(queue_.begin()->second));
        queue_.erase(queue_.begin());
        return res;
    }

private:
    std::mutex edit_queue_;
    std::condition_variable waiting_pop_;
    mutable std::map<std::chrono::system_clock::time_point, T> queue_;
};
