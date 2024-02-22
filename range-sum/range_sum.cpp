#include "range_sum.h"

uint64_t RangeSum(uint64_t from, uint64_t to, uint64_t step) {
    uint64_t result = 0;
    while (from < to) {
        result += from;
        from += step;
    }
    return result;
}
