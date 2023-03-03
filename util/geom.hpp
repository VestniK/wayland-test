#pragma once

#include <cstdint>

template <typename T>
struct basic_size {
  T width = {};
  T height = {};
};

template <typename T>
constexpr bool operator==(basic_size<T> lhs, basic_size<T> rhs) noexcept {
  return lhs.width == rhs.width && lhs.height == rhs.height;
}

using size = basic_size<int32_t>;
