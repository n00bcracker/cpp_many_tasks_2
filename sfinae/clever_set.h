#pragma once

#include <set>

template <class T>
class CleverSet {
public:
    bool Insert(const T& value) {
        return false;
    }

    bool Erase(const T& value) {
        return false;
    }

    bool Find(const T& value) const {
        return false;
    }

    size_t Size() const {
        return data_.size();
    }

private:
    std::set<T> data_;
};
