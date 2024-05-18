#pragma once

#include <algorithm>
#include <chrono>
#include <coroutine>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

using TimePoint = std::chrono::system_clock::time_point;
using Duration = std::chrono::system_clock::duration;

struct Promise;

class Coroutine {
public:
    using Handle = std::coroutine_handle<Promise>;
    using promise_type = Promise;

    struct Compare {
        bool operator()(const Coroutine& lhr, const Coroutine& rhr);
    };

    explicit Coroutine(Handle handle)
        : handle_{handle},
          is_started(false),
          added_time(std::chrono::system_clock::now()),
          next_time(added_time) {
    }

    Coroutine(const Coroutine&) = delete;
    Coroutine& operator=(const Coroutine&) = delete;

    Coroutine(Coroutine&& other)
        : handle_{other.handle_},
          is_started(other.is_started),
          added_time(other.added_time),
          next_time(other.next_time) {
        other.handle_ = nullptr;
    }
    Coroutine& operator=(Coroutine&& other) {
        if (this != &other) {
            Clear();
            handle_ = other.handle_;
            is_started = other.is_started;
            added_time = other.added_time;
            next_time = other.next_time;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~Coroutine() {
        Clear();
    }

    void Resume();

    bool Done() const {
        return handle_.done();
    }

    void Clear() {
        if (handle_) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

private:
    Handle handle_;

public:
    bool is_started;
    TimePoint added_time;
    TimePoint next_time;
};

struct Awaiter {
    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<>) {
    }
    void await_resume() {
        std::this_thread::sleep_until(time_point);
    }

    TimePoint time_point;
};

struct Promise {
    Coroutine get_return_object() {
        return Coroutine{Coroutine::Handle::from_promise(*this)};
    }
    std::suspend_always initial_suspend() {
        return {};
    }
    std::suspend_always final_suspend() noexcept {
        return {};
    }
    void unhandled_exception() {
    }
    void return_void() {
    }
    Awaiter await_transform(TimePoint time_point) {
        next_time = time_point;
        return {time_point};
    }
    Awaiter await_transform(Duration duration) {
        return await_transform(std::chrono::system_clock::now() + duration);
    }

    TimePoint next_time;
};

void Coroutine::Resume() {
    is_started = true;
    handle_.resume();
    const Promise& promise = handle_.promise();
    next_time = promise.next_time;
}

bool Coroutine::Compare::operator()(const Coroutine& lhr, const Coroutine& rhr) {
    if (lhr.is_started > rhr.is_started) {
        return true;
    } else if (lhr.is_started < rhr.is_started) {
        return false;
    }

    if (lhr.next_time > rhr.next_time) {
        return true;
    } else if (lhr.next_time < rhr.next_time) {
        return false;
    }

    if (lhr.added_time > rhr.added_time) {
        return true;
    } else {
        return false;
    }
}

class Scheduler {
public:
    template <class... Args>
    void AddTask(auto&& task, Args&&... args) {
        coroutines_.emplace_back(task(std::forward<Args>(args)...));
        std::push_heap(coroutines_.begin(), coroutines_.end(), Coroutine::Compare{});
    }

    bool Step() {
        if (coroutines_.empty()) {
            return false;
        }

        std::pop_heap(coroutines_.begin(), coroutines_.end(), Coroutine::Compare{});
        Coroutine c(std::move(coroutines_.back()));
        coroutines_.pop_back();

        c.Resume();
        if (!c.Done()) {
            coroutines_.emplace_back(std::move(c));
            std::push_heap(coroutines_.begin(), coroutines_.end(), Coroutine::Compare{});
        }

        return true;
    }

    void Run() {
        while (Step()) {
        }
    }

private:
    std::vector<Coroutine> coroutines_;
};
