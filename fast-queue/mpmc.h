#pragma once

#include <atomic>
#include <cstddef>
#include <thread>

template <class T>
class MPMCBoundedQueue {
private:
    struct Element {
        T value;
        std::atomic_size_t generation;
    };

public:
    explicit MPMCBoundedQueue(size_t size) : max_size_(size), bit_mask_(max_size_ - 1) {
        queue_ = new Element[size];
        for (size_t i = 0; i < max_size_; ++i) {
            queue_[i].generation = i;
        }
    }

    bool Enqueue(const T& value) {
        size_t tail = tail_;
        size_t index;
        do {
            index = tail & bit_mask_;
            if (queue_[index].generation.load(std::memory_order_acquire) == tail - bit_mask_) {
                return false;
            }
        } while (!tail_.compare_exchange_weak(tail, tail + 1));

        queue_[index].value = value;
        queue_[index].generation.fetch_add(1u, std::memory_order::release);
        return true;
    }

    bool Dequeue(T& data) {
        size_t head = head_;
        size_t index;
        do {
            index = head & bit_mask_;
            if (queue_[index].generation.load(std::memory_order_acquire) == head) {
                return false;
            }
        } while (!head_.compare_exchange_weak(head, head + 1));

        data = queue_[index].value;
        queue_[index].generation.fetch_add(bit_mask_, std::memory_order::release);
        return true;
    }

    ~MPMCBoundedQueue() {
        delete[] queue_;
    }

private:
    const size_t max_size_;
    const size_t bit_mask_;
    std::atomic_size_t head_ = 0;
    std::atomic_size_t tail_ = 0;
    Element* queue_ = nullptr;
};
