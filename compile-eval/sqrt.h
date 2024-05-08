#pragma once

template <bool C, class T, class F>
struct If {
    static const int kValue = T::kValue;
};

template <class T, class F>
struct If<false, T, F> {
    static const int kValue = F::kValue;
};

template <int L, int R, int N>
struct BinSearchSqrt {
    static const int kValue =
        If<((((L + R) / 2) < (N - 1) / ((L + R) / 2) + 1)), struct BinSearchSqrt<(L + R) / 2 + 1, R, N>,
           struct BinSearchSqrt<L, (L + R) / 2, N> >::kValue;
};

template <int L, int N>
struct BinSearchSqrt<L, L, N> {
    static const int kValue = L;
};

template <int L, int R>
struct BinSearchSqrt<L, R, 1> {
    static const int kValue = 1;
};

template <int N>
struct Sqrt {
    static const int kValue = BinSearchSqrt<0, N, N>::kValue;
};