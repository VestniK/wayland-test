#pragma once

#include <algorithm>
#include <span>

#include <util/io/input.hpp>
#include <util/io/output.hpp>

template <>
size_t io::input_traits<std::span<const std::byte>>::read(
    std::span<const std::byte>& in, std::span<std::byte> dest,
    std::error_code&) noexcept {
  const auto sz = std::min(in.size(), dest.size());
  std::ranges::copy(in.subspan(0, sz), dest.data());
  in = in.subspan(sz);
  return sz;
}
