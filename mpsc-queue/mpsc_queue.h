#pragma once

#include <stack>
#include <mutex>

template <class T>
class MPSCQueue {
public:
    // Push adds one element to stack top.
    // Safe to call from multiple threads.
    void Push(const T& value) {
        std::lock_guard guard{mutex_};
        stack_.push(value);
    }

    // Pop removes top element from the stack.
    // Not safe to call concurrently.
    std::pair<T, bool> Pop() {
        std::lock_guard guard{mutex_};
        if (stack_.empty()) {
            return {{}, false};
        }
        std::pair result{std::move(stack_.top()), true};
        stack_.pop();
        return result;
    }

    // DequeuedAll Pop's all elements from the stack and calls callback() for each.
    // Not safe to call concurrently with Pop()
    void DequeueAll(auto&& callback) {
        std::lock_guard guard{mutex_};
        while (!stack_.empty()) {
            callback(stack_.top());
            stack_.pop();
        }
    }

private:
    std::stack<T> stack_;
    std::mutex mutex_;
};
