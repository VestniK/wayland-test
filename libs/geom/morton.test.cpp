#include <catch2/catch_test_macros.hpp>

#include <libs/geom/morton.hpp>

TEST_CASE("morton::interleave_2", "[morton]") {
  SECTION("must interleave 2 adjancient bits") { REQUIRE(morton::interleave_2(0b0011u) == 0b1001u); }
  SECTION("must interleave 3 adjancient bits") { REQUIRE(morton::interleave_2(0b0000111u) == 0b1001001u); }
  SECTION("must interleave 5 adjancient bits") {
    REQUIRE(morton::interleave_2(0b00000011111u) == 0b1001001001001u);
  }
  SECTION("must interleave 9 adjancient bits") {
    REQUIRE(morton::interleave_2(0b111111111u) == 0b1001001001001001001001001u);
  }
  SECTION("must interleave 17 adjancient bits") {
    REQUIRE(
        morton::interleave_2(0b11111111111111111u) == 0b1001001001001001001001001001001001001001001001001u
    );
  }
  SECTION("must leave zeroes in 2 most significant bits after interleaving max "
          "allowed value") {
    constexpr uint32_t max_val = (1 << 21) - 1;
    REQUIRE((morton::interleave_2(max_val) >> 62) == 0);
  }
}
