#pragma once

#include <cstddef>
#include <utility>

#include <libs/geom/geom.hpp>

namespace img {

enum class pixel_fmt : size_t { grayscale = 1, rgb = 3, rgba = 4 };

constexpr size_t pixel_byte_size(pixel_fmt fmt) noexcept { return std::to_underlying(fmt); }

constexpr size_t bytes_size(size sz, pixel_fmt fmt) noexcept {
  return sz.height * sz.width * pixel_byte_size(fmt);
}

} // namespace img
