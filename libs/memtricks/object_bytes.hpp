#pragma once

#include <span>
#include <type_traits>

template <typename T>
  requires std::is_standard_layout_v<T>
std::span<std::byte> object_bytes(T& obj) {
  return std::as_writable_bytes(std::span{&obj, 1});
}

template <typename T>
std::span<const std::byte> object_bytes(const T& obj) {
  return std::as_bytes(std::span{&obj, 1});
}
