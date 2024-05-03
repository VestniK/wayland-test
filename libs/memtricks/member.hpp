#pragma once

#include <cstddef>

template <typename C, typename M>
using member_ptr = M C::*;

template <typename C, typename M>
constexpr ptrdiff_t member_offset(member_ptr<C, M> member) noexcept {
  const std::byte* null_ptr = nullptr;
  return reinterpret_cast<const std::byte*>(
             &(static_cast<const C*>(nullptr)->*member)) -
         null_ptr;
}
