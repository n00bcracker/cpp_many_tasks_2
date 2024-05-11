#pragma once

#include <cstddef>
#include <iterator>

template <typename InIterator>
void AdvanceImpl(InIterator& iterator, ptrdiff_t n, std::input_iterator_tag) {
    while (n > 0) {
        ++iterator;
        --n;
    }
}

template <typename OutIterator>
void AdvanceImpl(OutIterator& iterator, ptrdiff_t n, std::output_iterator_tag) {
    while (n > 0) {
        ++iterator;
        --n;
    }
}

template <typename FWDIterator>
void AdvanceImpl(FWDIterator& iterator, ptrdiff_t n, std::forward_iterator_tag) {
    while (n > 0) {
        ++iterator;
        --n;
    }
}

template <typename BDIterator>
void AdvanceImpl(BDIterator& iterator, ptrdiff_t n, std::bidirectional_iterator_tag) {
    while (n > 0) {
        ++iterator;
        --n;
    }

    while (n < 0) {
        --iterator;
        ++n;
    }
}

template <typename RAIterator>
void AdvanceImpl(RAIterator& iterator, ptrdiff_t n, std::random_access_iterator_tag) {
    iterator += n;
}

template <class Iterator>
void Advance(Iterator& iterator, ptrdiff_t n) {
    AdvanceImpl(iterator, n, typename std::iterator_traits<Iterator>::iterator_category());
}
