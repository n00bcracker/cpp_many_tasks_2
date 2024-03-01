#pragma once

#include <optional>

template <class T>
class BufferedChannel {
public:
    explicit BufferedChannel(uint32_t size) {
    }

    void Send(const T& value) {
    }

    void Send(T&& value) {
    }

    std::optional<T> Recv() {
        return std::nullopt;
    }

    void Close() {
    }
};
