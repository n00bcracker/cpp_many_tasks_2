#pragma once

#include <cstdint>

constexpr int HashImpl(const char* s, uint64_t p, uint64_t mod) {
    return *s == '\0' ? 0 : (HashImpl(s + 1, p, mod) * p % mod + *s) % mod;
}

constexpr int Hash(const char* s, int p, int mod) {
    return HashImpl(s, p, mod);
}