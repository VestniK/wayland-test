#pragma once

#include <cstddef>

template <typename T>
struct arity {
  static constexpr size_t value = 1;
};

template <typename T, size_t N>
struct arity<T[N]> {
  static constexpr size_t value = N;
};

template <typename T>
constexpr size_t arity_v = arity<T>::value;
