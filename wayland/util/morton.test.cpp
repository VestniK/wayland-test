#include <catch2/catch.hpp>

#include <wayland/util/morton.hpp>

/*
0011 << 2 = 1100
0011 | 1100 = 1111
1111 & 1001 = 1001
*/

constexpr uint64_t interleave_2(uint32_t val) noexcept {
  constexpr uint64_t shifts[] = {32, 16, 8, 4, 2};
  // clang-format off
  constexpr uint64_t masks[] = {
    0xffff'0000'0000'ffff,
    0x00ff'0000'ff00'00ff,
    0xf00f'00f0'0f00'f00f,
    0x30c3'0c30'c30c'30c3,
    0x9249'2492'4924'9249
  };
  // clang-format on

  uint64_t r = val;
  r = (r | (r << shifts[0])) & masks[0];
  r = (r | (r << shifts[1])) & masks[1];
  r = (r | (r << shifts[2])) & masks[2];
  r = (r | (r << shifts[3])) & masks[3];
  r = (r | (r << shifts[4])) & masks[4];
  return r;
}

TEST_CASE("interleave_2", "[morton]") {
  SECTION("must interleave 2 adjancient bits") {
    REQUIRE(interleave_2(0b0011u) == 0b1001u);
  }
  SECTION("must interleave 3 adjancient bits") {
    REQUIRE(interleave_2(0b0000111u) == 0b1001001u);
  }
  SECTION("must interleave 5 adjancient bits") {
    REQUIRE(interleave_2(0b00000011111u) == 0b1001001001001u);
  }
  SECTION("must interleave 9 adjancient bits") {
    REQUIRE(interleave_2(0b111111111u) == 0b1001001001001001001001001u);
  }
  SECTION("must interleave 17 adjancient bits") {
    REQUIRE(interleave_2(0b11111111111111111u) ==
            0b1001001001001001001001001001001001001001001001001u);
  }
}
