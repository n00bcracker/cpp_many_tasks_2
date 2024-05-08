#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

template <typename T, size_t N, size_t... I>
constexpr auto SliceImpl(const std::array<T, N>& a, std::index_sequence<I...>) {
    return std::array<T, N - 1>{a[I + 1]...};
}

template <typename T, std::size_t N, typename Indices = std::make_index_sequence<N - 1>>
constexpr auto Slice(const std::array<T, N>& a) {
    return SliceImpl(a, Indices{});
}

template <size_t K, size_t N, size_t... I1, size_t... I2>
constexpr auto SubmatrixImpl(std::index_sequence<I1...> seq1,
                             const std::array<std::array<int, N>, N>& a,
                             std::index_sequence<I2...> seq2) {
    return std::array<std::array<int, N - 1>, N - 1>{Slice(a[I1])..., Slice(a[I2 + K + 1])...};
}

template <size_t K, size_t N>
constexpr auto Submatrix(const std::array<std::array<int, N>, N>& a) {
    auto first_part = std::make_index_sequence<K>{};
    auto second_part = std::make_index_sequence<N - K - 1>{};
    return SubmatrixImpl<K>(first_part, a, second_part);
}

template <size_t N>
constexpr int Determinant(const std::array<std::array<int, N>, N>& a);

template <size_t N, size_t... I>
constexpr auto Apply(const std::array<std::array<int, N>, N>& a, std::index_sequence<I...>) {
    return std::array<int64_t, N>{Determinant(Submatrix<I>(a))...};
}

template <size_t N>
constexpr int Determinant(const std::array<std::array<int, N>, N>& a) {
    int64_t res = 0;
    int64_t m = 1;

    const auto indexes = std::make_index_sequence<N>{};
    const std::array<int64_t, N> minors = Apply(a, indexes);

    for (size_t i = 0; i < a.size(); ++i) {
        res += m * a[i][0] * minors[i];
        m *= -1;
    }

    return res;
}

template <>
constexpr int Determinant<1>(const std::array<std::array<int, 1>, 1>& a) {
    return a[0][0];
}
