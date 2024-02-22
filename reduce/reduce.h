#pragma once

#include <numeric>
#include <iterator>

template <std::random_access_iterator Iterator, class T>
T Reduce(Iterator first, Iterator last, const T& init, auto func) {
    return std::reduce(first, last, init, func);
}
