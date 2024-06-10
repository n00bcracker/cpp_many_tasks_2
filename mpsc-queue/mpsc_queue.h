#pragma once

#include <atomic>
#include <optional>
#include <utility>

template <class T>
class MPSCQueue {
public:
    // Push adds one element to stack top.
    // Safe to call from multiple threads.
    void Push(T value) {
        Node* old_head = head_;
        Node* new_head = new Node{std::move(value), old_head};
        while (!head_.compare_exchange_weak(old_head, new_head)) {
            old_head = head_;
            new_head->next = old_head;
        }
    }

    // Pop removes top element from the stack.
    // Not safe to call concurrently.
    std::optional<T> Pop() {
        if (!head_) {
            return std::nullopt;
        }

        Node* old_head = head_;
        head_ = head_.load()->next;
        std::optional<T> res = std::move(old_head->value);
        delete old_head;
        return res;
    }

    // DequeuedAll Pop's all elements from the stack and calls callback() for each.
    // Not safe to call concurrently with Pop()
    void DequeueAll(auto&& callback) {
        Node* old_head = head_;
        while (!head_.compare_exchange_weak(old_head, nullptr)) {
            old_head = head_;
        }

        while (old_head) {
            Node* node = old_head;
            old_head = old_head->next;
            callback(std::move(node->value));
            delete node;
        }
    }

    ~MPSCQueue() {
        DequeueAll([](const auto) {});
    }

private:
    struct Node {
        T value;
        Node* next;
    };

    std::atomic<Node*> head_ = nullptr;
};
