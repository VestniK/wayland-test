#pragma once

#include <cassert>
#include <cstdint>

namespace morton {

constexpr uint64_t interleave_1(uint32_t val) noexcept {
  constexpr uint64_t shifts[] = {16, 8, 4, 2, 1};
  // clang-format off
  constexpr uint64_t masks[] = {
    0x0000'ffff'0000'ffff,
    0x00ff'00ff'00ff'00ff,
    0x0f0f'0f0f'0f0f'0f0f,
    0x3333'3333'3333'3333,
    0x5555'5555'5555'5555
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

  assert((val >> 21) == 0);
  uint64_t r = val;
  r = (r | (r << shifts[0])) & masks[0];
  r = (r | (r << shifts[1])) & masks[1];
  r = (r | (r << shifts[2])) & masks[2];
  r = (r | (r << shifts[3])) & masks[3];
  r = (r | (r << shifts[4])) & masks[4];
  return r;
}

constexpr uint64_t code(uint32_t x, uint32_t y) noexcept { return interleave_1(x) | (interleave_1(y) << 1); }

constexpr uint64_t code(uint32_t x, uint32_t y, uint32_t z) noexcept {
  return interleave_2(x) | (interleave_2(y) << 1) | (interleave_2(z) << 2);
}

} // namespace morton
