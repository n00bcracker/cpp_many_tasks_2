#pragma once

template <unsigned a, unsigned b>
struct Pow {
    static const unsigned kValue = Pow<a, b - 1>::kValue * a;
};

template <unsigned a>
struct Pow<a, 0> {
    static const unsigned kValue = 1;
};