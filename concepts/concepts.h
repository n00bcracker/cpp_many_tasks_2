#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

template <class P, class T>
concept Predicate = requires(P p, T t) {
    { p(t) } -> std::same_as<bool>;
};

template <typename T>
concept NotVoidable = !std::is_void<T>::value;

template <class T>
concept Indexable = requires(T t) {
    { std::begin(t) } -> std::random_access_iterator;
    { std::end(t) } -> std::random_access_iterator;
}
|| requires(T t, size_t i) {
    { t[i] } -> NotVoidable;
};

template <typename T>
concept Stringable = requires(T t) {
    { std::string(t) } -> std::same_as<std::string>;
};

template <typename T>
struct JsonSerializable {
    static constexpr bool kValue = false;
};

template <std::integral T>
struct JsonSerializable<T> {
    static constexpr bool kValue = true;
};

template <std::floating_point T>
struct JsonSerializable<T> {
    static constexpr bool kValue = true;
};

template <Stringable T>
struct JsonSerializable<T> {
    static constexpr bool kValue = true;
};

template <typename T>
struct JsonSerializable<std::optional<T>> : JsonSerializable<T> {};

template <typename T, typename... Args>
struct JsonSerializable<std::variant<T, Args...>> {
    static constexpr bool kValue =
        JsonSerializable<T>::kValue && JsonSerializable<std::variant<Args...>>::kValue;
};

template <typename T>
struct JsonSerializable<std::variant<T>> : JsonSerializable<T> {};

template <typename T>
concept IsPair = requires(T t) {
    t.first;
    t.second;
};

template <std::ranges::range R>
requires(!Stringable<R> && !IsPair<std::ranges::range_value_t<R>>) struct JsonSerializable<R>
    : JsonSerializable<std::ranges::range_value_t<R>> {
};

template <std::ranges::range R>
requires(!Stringable<R> && Stringable<typename std::tuple_element<
                               0, std::ranges::range_value_t<R>>::type>) struct JsonSerializable<R>
    : JsonSerializable<typename std::tuple_element<1, std::ranges::range_value_t<R>>::type> {
};

template <class T>
concept SerializableToJson = JsonSerializable<T>::kValue;
