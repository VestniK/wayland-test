#pragma once

#include <cstdint>

template<typename T>
struct basic_point {
  T x = 0;
  T y = 0;
};

using point = basic_point<int32_t>;

template<typename T>
struct basic_size {
  T width = {};
  T height = {};
};

template<typename T>
constexpr bool operator== (basic_size<T> lhs, basic_size<T> rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height;
}

using size = basic_size<int32_t>;

struct rect {
  point top_left;
  ::size size;
};
