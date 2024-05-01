#include <catch2/catch_test_macros.hpp>

#include "sum.h"

TEST_CASE("Base") {
    REQUIRE(GetSum(5, 6) == 11);
    REQUIRE(GetSum(220, 8) == 228);
    REQUIRE(GetSum(1337, 151) == 1488);
}

TEST_CASE("Overflow") {
    REQUIRE(GetSum(2'000'000'000, 2'000'000'000) == 4'000'000'000);
}