#include "range_sum.h"
#include <cstdint>

uint64_t RangeSum(uint64_t from, uint64_t to, uint64_t step) {
    if (to > from) {
        uint64_t n = (to - 1 - from) / step + 1;
        return (2 * from + (n - 1) * step) * n / 2;
    } else {
        return 0;
    }
}
