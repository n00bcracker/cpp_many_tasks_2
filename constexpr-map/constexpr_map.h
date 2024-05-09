#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>
template <class K, class V, int MaxSize = 8>
class ConstexprMap {
public:
    constexpr V& operator[](const K& key) {
        for (size_t i = 0; i < size_; ++i) {
            if (map_[i].first == key) {
                return map_[i].second;
            }
        }

        if (size_ < map_.size()) {
            map_[size_].first = key;
            return map_[size_++].second;
        } else {
            throw std::runtime_error("Map is full");
        }
    }

    constexpr const V& operator[](const K& key) const {
        for (size_t i = 0; i < size_; ++i) {
            if (map_[i].first == key) {
                return map_[i].second;
            }
        }

        throw std::runtime_error("Key is missing");
    }

    constexpr bool Erase(const K& key) {
        for (size_t i = 0; i < size_; ++i) {
            if (map_[i].first == key) {
                ShiftValues(i);
                --size_;
                return true;
            }
        }

        return false;
    }

    constexpr bool Find(const K& key) const {
        for (size_t i = 0; i < size_; ++i) {
            if (map_[i].first == key) {
                return true;
            }
        }

        return false;
    }

    constexpr size_t Size() const {
        return size_;
    }

    constexpr std::pair<K, V>& GetByIndex(size_t pos) {
        if (pos < size_) {
            return map_[pos];
        } else {
            throw std::runtime_error("Wrong index");
        }
    }

    constexpr const std::pair<K, V>& GetByIndex(size_t pos) const {
        if (pos < size_) {
            return map_[pos];
        } else {
            throw std::runtime_error("Wrong index");
        }
    }

private:
    constexpr void ShiftValues(size_t deleted_index) {
        for (size_t i = deleted_index + 1; i < size_; ++i) {
            map_[i - 1] = map_[i];
        }
    }

    std::array<std::pair<K, V>, MaxSize> map_ = {};
    size_t size_ = 0;
};
