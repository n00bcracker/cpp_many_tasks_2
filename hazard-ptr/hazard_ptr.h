#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <memory>
#include <unordered_set>

inline thread_local std::atomic<void*> hazard_ptr(nullptr);

struct ThreadState {
    std::atomic<void*>* ptr;
};

inline std::mutex threads_lock;
inline std::unordered_set<ThreadState*> threads;

void RegisterThread();
void UnregisterThread();

template <class T>
inline T* Acquire(std::atomic<T*>* ptr) {
    auto* value = ptr->load();

    do {
        hazard_ptr.store(value);

        auto* new_value = ptr->load();
        if (new_value == value) {
            return value;
        }

        value = new_value;
    } while (true);
}

inline void Release() {
    hazard_ptr.store(nullptr);
}

struct RetiredPtr {
    void* value;
    std::function<void()> deleter;
    RetiredPtr* next;
};

inline std::atomic<RetiredPtr*> free_list = nullptr;
inline std::atomic_size_t approximate_free_list_size = 0;
constexpr size_t kFreeListMaxSize = 15;

inline std::mutex scan_lock;

inline void ScanFreeList() {
    approximate_free_list_size.store(0);

    if (!scan_lock.try_lock()) {
        return;
    }

    std::lock_guard lock(scan_lock, std::adopt_lock);
    auto* retired = free_list.exchange(nullptr);
    std::vector<void*> hazard;
    {
        std::lock_guard guard{threads_lock};
        for (auto* thread : threads) {
            if (auto* ptr = thread->ptr->load()) {
                hazard.push_back(ptr);
            }
        }
    }

    std::sort(hazard.begin(), hazard.end());

    while (retired) {
        auto* node = retired;
        retired = retired->next;
        if (!std::binary_search(hazard.begin(), hazard.end(), node->value)) {
            node->deleter();
            delete node;
        } else {
            node->next = free_list.load();
            while (!free_list.compare_exchange_weak(node->next, node)) {
            }
        }
    }
}

template <class T, class Deleter = std::default_delete<T>>
inline void Retire(T* value, Deleter deleter = {}) {
    RetiredPtr* retired_ptr =
        new RetiredPtr{value, [deleter, value]() { deleter(value); }, free_list.load()};

    while (!free_list.compare_exchange_weak(retired_ptr->next, retired_ptr)) {
    }

    approximate_free_list_size.fetch_add(1);

    if (approximate_free_list_size > kFreeListMaxSize) {
        ScanFreeList();
    }
}
