#pragma once

#include <functional>
#include <span>

#include <libs/img/pixel_fmt.hpp>

namespace img {

class reader {
public:
  reader() noexcept = default;
  reader(::size sz, pixel_fmt fmt, std::move_only_function<size_t(std::span<std::byte>)> read_pixels)
      : read_pixels_{std::move(read_pixels)}, sz_{sz}, fmt_{fmt} {}

  ::size size() const noexcept { return sz_; }
  pixel_fmt format() const noexcept { return fmt_; }

  size_t pixels_size() const noexcept { return bytes_size(sz_, fmt_); }

  size_t read_pixels(std::span<std::byte> dest) { return read_pixels_(dest); }

private:
  std::move_only_function<size_t(std::span<std::byte>)> read_pixels_;
  ::size sz_;
  pixel_fmt fmt_;
};

} // namespace img
