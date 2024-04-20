#include "matrix.h"

#include <vector>
#include <limits>
#include <random>
#include <deque>
#include <string>
#include <ranges>
#include <utility>
#include <functional>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

struct Int {
    int64_t x;
    static inline auto mul_counter = 0;

    explicit Int(int64_t x = 0) : x{x} {
    }

    static void Reset() {
        mul_counter = 0;
    }

    Int& operator+=(Int rhs) {
        x += rhs.x;
        return *this;
    }

    Int& operator-=(Int rhs) {
        x -= rhs.x;
        return *this;
    }

    Int& operator*=(Int rhs) {
        x *= rhs.x;
        ++mul_counter;
        return *this;
    }
};

Int operator*(Int a, Int b) {
    ++Int::mul_counter;
    return Int{a.x * b.x};
}

Int operator+(Int a, Int b) {
    return Int{a.x + b.x};
}

Int operator-(Int a, Int b) {
    return Int{a.x - b.x};
}

Int operator-(Int a) {
    return Int{-a.x};
}

TEST_CASE("Basic ops") {
    Matrix<int> left(2, 3);
    Matrix<int> right(2, 3);
    Matrix<int> sub(2, 3);
    for (auto i : std::views::iota(0, 2)) {
        for (auto j : std::views::iota(0, 3)) {
            left(i, j) = i;
            right(i, j) = j;
            sub(i, j) = (i == j);
        }
    }
    Matrix test = left + right - sub;
    REQUIRE(test.Rows() == 2);
    REQUIRE(test.Columns() == 3);
    for (auto i : std::views::iota(0, 2)) {
        for (auto j : std::views::iota(0, 3)) {
            REQUIRE(test(i, j) == i + j - (i == j));
        }
    }
}

TEST_CASE("Multipling") {
    Matrix<int> left{{1, 2, 3}, {4, 5, 6}};
    Matrix<int> right{{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};
    Matrix<int> test = left * right;
    std::vector<std::vector<int>> expected{{38, 44, 50, 56}, {83, 98, 113, 128}};

    REQUIRE(test.Rows() == 2);
    REQUIRE(test.Columns() == 4);
    for (auto i : std::views::iota(0u, test.Rows())) {
        for (auto j : std::views::iota(0u, test.Columns())) {
            REQUIRE(test(i, j) == expected[i][j]);
        }
    }
}

TEST_CASE("Mixed ops") {
    constexpr auto kInf = std::numeric_limits<uint64_t>::max();
    Matrix<uint64_t> left{{1, 1}, {1, 1}};
    Matrix<uint64_t> right{{kInf, kInf}, {kInf, kInf}};
    Matrix<uint64_t> mul{{1, 2}, {3, 4}};

    Matrix<uint64_t> result = mul * (left + right);
    REQUIRE(result.Rows() == 2);
    REQUIRE(result.Columns() == 2);
    for (auto i : std::views::iota(0, 2)) {
        for (auto j : std::views::iota(0, 2)) {
            REQUIRE(result(i, j) == 0);
        }
    }
}

TEST_CASE("Simple order") {
    Int::Reset();
    Matrix<Int> first(50, 5);
    Matrix<Int> second(5, 100);
    Matrix<Int> third(100, 10);
    Matrix<Int> result = first * second * third;
    REQUIRE(Int::mul_counter == 7500);
}

TEST_CASE("Exceptions") {
    Matrix<int> first(2, 3);
    Matrix<int> second(3, 4);
    Matrix<int> third(4, 6);
    Matrix<int> fourth(5, 7);
    REQUIRE_THROWS_AS(Matrix<int>(first * second * third * fourth), std::runtime_error);
}

TEST_CASE("Chain") {
    Int::Reset();
    Matrix<Int> a(40, 20);
    Matrix<Int> b(20, 30);
    Matrix<Int> c(30, 10);
    Matrix<Int> d(10, 30);
    Matrix<Int> e(40, 20);
    Matrix<Int> f(20, 30);
    Matrix<Int> result = a * b * c * d + e * f;
    REQUIRE(Int::mul_counter == 26000 + 24000);
}

Matrix<Int> TestMul(const Matrix<Int>& lhs, const Matrix<Int>& rhs) {
    Matrix<Int> result(lhs.Rows(), rhs.Columns());
    for (auto i : std::views::iota(0u, lhs.Rows())) {
        for (auto j : std::views::iota(0u, rhs.Columns())) {
            Int sum{};
            for (auto k : std::views::iota(0u, lhs.Columns())) {
                sum += lhs(i, k) * rhs(k, j);
            }
            result(i, j) = sum;
        }
    }
    return result;
}

template <size_t... I>
auto MultiplyImpl(const auto& range, std::index_sequence<I...>) {
    return Matrix<Int>{(... * range[I])};
}

template <size_t N>
auto Multiply(const auto& range) {
    return MultiplyImpl(range, std::make_index_sequence<N>{});
}

TEST_CASE("Stress") {
    constexpr auto kMatrixCount = 10;
    std::mt19937 gen{7'835'675u};
    std::uniform_int_distribution dist_sizes{5, 100};
    std::uniform_int_distribution dist_elems{-5, 5};
    auto ok_counts = {528'658, 146'820, 129'235, 148'710, 179'753,
                      233'154, 312'672, 130'800, 546'624, 165'880};
    for (auto ok_count_true : ok_counts) {
        std::vector<size_t> sizes(kMatrixCount + 1);
        for (auto& cur : sizes) {
            cur = dist_sizes(gen);
        }

        std::deque<Matrix<Int>> matrices;
        for (auto q : std::views::iota(0, kMatrixCount)) {
            Matrix<Int> cur(sizes[q], sizes[q + 1]);
            for (auto i : std::views::iota(0u, cur.Rows())) {
                for (auto j : std::views::iota(0u, cur.Columns())) {
                    cur(i, j).x = dist_elems(gen);
                }
            }
            matrices.push_back(std::move(cur));
        }

        Int::Reset();
        auto ok = matrices.front();
        for (const auto& matrix : matrices | std::views::drop(1)) {
            ok = TestMul(ok, matrix);
        }

        Int::Reset();
        auto test = Multiply<kMatrixCount>(matrices);
        auto ok_count = Int::mul_counter;
        REQUIRE(ok_count == ok_count_true);
        REQUIRE(ok.Rows() == test.Rows());
        REQUIRE(ok.Columns() == test.Columns());

        for (auto i : std::views::iota(0u, ok.Rows())) {
            for (auto j : std::views::iota(0u, ok.Columns())) {
                REQUIRE(ok(i, j).x == test(i, j).x);
            }
        }
    }
}

template <class T, class U>
concept Multipliable = requires(T a, U b) { std::multiplies{}(a, b); };

template <class T>
concept MultipliableWithMatrix = Multipliable<T, Matrix<float>> || Multipliable<Matrix<float>, T>;

TEST_CASE("Glue doesn't break *") {
    STATIC_CHECK(MultipliableWithMatrix<Matrix<float>>);
    STATIC_CHECK_FALSE(MultipliableWithMatrix<std::string>);
    STATIC_CHECK_FALSE(MultipliableWithMatrix<std::vector<int>>);
    STATIC_CHECK_FALSE(MultipliableWithMatrix<std::pair<int, double>>);
    STATIC_CHECK_FALSE(MultipliableWithMatrix<std::deque<Matrix<float>>>);
}
