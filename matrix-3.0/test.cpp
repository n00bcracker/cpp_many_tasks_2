#include "interpreter.h"

#include <map>
#include <string>
#include <iostream>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

using Map = std::map<std::string, uint64_t>;

TEST_CASE("Simple") {
    Map expected{{"A", 1}, {"B", 2}, {"C", 3}};
    auto program = R"(
A = 1
B = 2
C = A + B
)";
    REQUIRE(expected == Run(program));
}

TEST_CASE("Simple loop") {
    Map expected{{"mul", 1ull << 62}, {"inc1", 61}};
    auto program = R"(
mul = 1
inc1=-1

for 62
    mul *= 2

    inc1+=1
    )";
    REQUIRE(expected == Run(program));
}

TEST_CASE("Fibonacci") {
    Map expected{{"A", 1298777728820984005ull},
                 {"B", 5035488507601418376ull},
                 {"C", 5035488507601418376ull}};
    auto program = R"(
A= 1
B =1
for 100
    C = A
    C += B
    A = B
    B = C
    )";
    REQUIRE(expected == Run(program));
}

TEST_CASE("Inner loop") {
    Map expected{{"a1", 1000000000000000000ull}, {"a2", 16183169460410122240ull}};
    auto program = R"(
for 1000000000
    for 1000000000
        a1=a1+1
        a2+= a1)";
    REQUIRE(expected == Run(program));
}

TEST_CASE("Big loop") {
    Map expected{{"mul", 1}, {"s", 12114676451515301888ull}};
    auto program = R"(

mul = 1
for 1000000000000000000
    s -= mul + 5
    mul *= 17239

for 1000000000000000000
    mul = mul*9396186536994232423)";
    REQUIRE(expected == Run(program));
}

TEST_CASE("More loops") {
    Map expected{{"i", 0}, {"j", 0}, {"k", 0}, {"just_some_sum", 4852594790647124144ull}};
    auto program = R"(
for 100000
    i += 1
    for 100001
        k -= k
        j += 1
        for 99999
            just_some_sum += i + 2 * j + k * 3 - (-1)
            k += 1

    j = 0
i = 0
k = 0)";
    REQUIRE(expected == Run(program));
}

TEST_CASE("Some calculations") {
    Map expected{{"A", 11368981}, {"B", 2}, {"C", 0}, {"D", 18446744073709551324ull}};
    auto program = R"(
A = 1
B = 2
C = 3
D = A + 2*(-(B + C)*3)*10 + 7
for 0
    D = 0
C -= A + B

for 42
    for 42
        A += (B + C)* 5 + 11 * (-D * 2 + 1)
)";
    REQUIRE(expected == Run(program));
}

TEST_CASE("Error 1") {
    auto program = R"(
A = 7
B = (123 + )";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}

TEST_CASE("Error 2") {
    auto program = R"(
for 7
        for 5
            A = 3
B = 4)";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}

TEST_CASE("Error 3") {
    auto program = R"(
A = 5 + for)";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}

TEST_CASE("Error 4") {
    auto program = R"(
    for 37
B = 3)";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}

TEST_CASE("Error 5") {
    auto program = R"(
A += 7 + 274874a)";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}

TEST_CASE("Error 6") {
    auto program = R"(
for 1
  for 2
    for 6
      B += 9)";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}

TEST_CASE("Error 7") {
    auto program = "_ = _";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}

TEST_CASE("Error 8") {
    auto program = R"(
a = 5
for 7
    b = 0
        c = 3)";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}

TEST_CASE("Error 9") {
    auto program = "a = b = 3";
    REQUIRE_THROWS_AS(Run(program), std::exception);
}
