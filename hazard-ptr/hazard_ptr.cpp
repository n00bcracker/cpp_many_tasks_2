#include "hazard_ptr.h"
#include <mutex>

void RegisterThread() {
    ThreadState* state_ptr = new ThreadState{&hazard_ptr};
    std::lock_guard lock(threads_lock);
    threads.emplace(state_ptr);
}

void UnregisterThread() {
    size_t threads_size;
    {
        std::lock_guard lock(threads_lock);
        for (auto it = threads.begin(); it != threads.end(); ++it) {
            auto* state_ptr = *it;
            if (state_ptr->ptr == &hazard_ptr) {
                it = threads.erase(it);
                delete state_ptr;
                break;
            }
        }

        threads_size = threads.size();
    }

    if (threads_size == 0) {
        ScanFreeList();
    }
}
