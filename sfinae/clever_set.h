#pragma once

#include <cstddef>
#include <functional>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <utility>

template <class T, typename Enable = void>
class CleverSet {
public:
    bool Insert(const T& value) {
        return data_.insert(&value).second;
    }

    bool Erase(const T& value) {
        if (data_.find(&value) != data_.end()) {
            data_.erase(&value);
            return true;
        } else {
            return false;
        }
    }

    bool Find(const T& value) const {
        if (data_.find(&value) != data_.end()) {
            return true;
        } else {
            return false;
        }
    }

    size_t Size() const {
        return data_.size();
    }

private:
    std::unordered_set<const T*> data_;
};

template <typename T, typename Test = size_t>
struct IsHashable {
    static constexpr bool kValue = false;
};

template <typename T>
struct IsHashable<T, decltype(std::hash<T>()(std::declval<T>()))> {
    static constexpr bool kValue = true;
};

template <typename T>
struct IsEqualable {
    template <typename F, decltype(std::declval<F>() != std::declval<F>()) = true>
    static bool Test1(const F&);

    static void Test1(...);

    template <typename F, decltype(std::declval<F>() == std::declval<F>()) = true>
    static bool Test2(const F&);

    static void Test2(...);

    static constexpr bool kValue = std::is_same<bool, decltype(Test1(std::declval<T>()))>::value ||
                                   std::is_same<bool, decltype(Test2(std::declval<T>()))>::value;
};

template <class T>
class CleverSet<T, std::enable_if_t<IsHashable<T>::kValue && IsEqualable<T>::kValue>> {
public:
    bool Insert(const T& value) {
        return data_.insert(value).second;
    }

    bool Erase(const T& value) {
        if (data_.find(value) != data_.end()) {
            data_.erase(value);
            return true;
        } else {
            return false;
        }
    }

    bool Find(const T& value) const {
        if (data_.find(value) != data_.end()) {
            return true;
        } else {
            return false;
        }
    }

    size_t Size() const {
        return data_.size();
    }

private:
    std::unordered_set<T> data_;
};

template <typename T>
struct IsComparable {
    template <typename F, decltype(std::greater<>()(std::declval<F>(), std::declval<F>())) = true>
    static bool Test1(const F&);

    static void Test1(...);

    template <typename F, decltype(std::less<>()(std::declval<F>(), std::declval<F>())) = true>
    static bool Test2(const F&);

    static void Test2(...);

    static constexpr bool kValue = std::is_same<bool, decltype(Test1(std::declval<T>()))>::value ||
                                   std::is_same<bool, decltype(Test2(std::declval<T>()))>::value;
};

template <typename T, typename = bool>
struct Compare {
    bool operator()(const T& lhr, const T& rhr) const {
        return std::less<>()(lhr, rhr);
    }
};

template <typename T>
struct Compare<T, decltype(IsComparable<T>::Test1(std::declval<T>()))> {
    bool operator()(const T& lhr, const T& rhr) const {
        return std::greater<>()(rhr, lhr);
    }
};

template <class T>
class CleverSet<T, std::enable_if_t<IsComparable<T>::kValue &&
                                    !(IsHashable<T>::kValue && IsEqualable<T>::kValue)>> {
public:
    bool Insert(const T& value) {
        return data_.insert(value).second;
    }

    bool Erase(const T& value) {
        const auto it = data_.find(value);
        if (it != data_.end()) {
            data_.erase(value);
            return true;
        } else {
            return false;
        }
    }

    bool Find(const T& value) const {
        if (data_.find(value) != data_.end()) {
            return true;
        } else {
            return false;
        }
    }

    size_t Size() const {
        return data_.size();
    }

private:
    std::set<T, Compare<T>> data_;
};
