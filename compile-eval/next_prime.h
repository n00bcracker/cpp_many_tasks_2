#pragma once

constexpr int NextPrimeImpl(int x);

constexpr bool IsPrime(int x) {
    int i = 3;
    while (i * i <= x) {
        if (x % i == 0) {
            return false;
        } else {
            i = NextPrimeImpl(i);
        }
    }

    return true;
}

constexpr int NextPrimeImpl(int x) {
    if (x == 0) {
        return 2;
    } else if (x == 1) {
        return 2;
    } else if (x == 2) {
        return 3;
    } else {
        int res = 0;
        int i = x % 2 == 0 ? x + 1 : x + 2;
        for (;; i += 2) {
            if (IsPrime(i)) {
                res = i;
                break;
            }
        }

        return res;
    }
}

constexpr int NextPrime(int x) {
    return NextPrimeImpl(x - 1);
}