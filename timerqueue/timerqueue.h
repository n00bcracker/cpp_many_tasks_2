#pragma once

#include <chrono>

template <class T>
class TimerQueue {
public:
    template <class... Args>
    void Add(std::chrono::system_clock::time_point at, Args&&... args) {
    }

    T Pop() {
        return {};
    }
};
