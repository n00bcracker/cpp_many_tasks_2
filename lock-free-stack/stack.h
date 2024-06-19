#pragma once

#include "../hazard-ptr/hazard_ptr.h"

#include <mutex>

template <class T>
class Stack {
public:
    void Push(const T& value) {
        Node* old_head = head_;
        Node* new_head = new Node{value, old_head};
        while (!head_.compare_exchange_weak(old_head, new_head)) {
            new_head->next = old_head;
        }
    }

    bool Pop(T* value) {
        Node* old_head = Acquire(&head_);
        if (!old_head) {
            return false;
        }

        while (!head_.compare_exchange_weak(old_head, old_head->next)) {
            old_head = Acquire(&head_);
            if (!old_head) {
                return false;
            }
        }

        *value = std::move(old_head->value);
        Release();
        Retire(old_head);
        return true;
    }

    void Clear() {
        Node* old_head = head_;
        while (!head_.compare_exchange_weak(old_head, nullptr)) {
            old_head = head_;
        }

        while (old_head) {
            Node* node = old_head;
            old_head = old_head->next;
            Retire(node);
        }
    }

    ~Stack() {
        Clear();
    }

private:
    struct Node {
        T value;
        Node* next;
    };

    std::atomic<Node*> head_ = nullptr;
};
