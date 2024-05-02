#pragma once

#include <memory>
#include <span>

#include <thinsys/io/io.hpp>
#include <util/geom.hpp>

namespace img {

class image {
public:
  image() noexcept = default;
  image(std::unique_ptr<std::byte[]> data, ::size sz) noexcept
      : sz_{sz}, data_{std::move(data)} {}

  std::span<const std::byte> data() const noexcept {
    return {data_.get(), sz_.height * sz_.width * pixel_size_};
  }

  ::size size() const noexcept { return sz_; }

private:
  static constexpr size_t pixel_size_ = 8 * 3;

private:
  ::size sz_;
  std::unique_ptr<std::byte[]> data_;
};

image load(thinsys::io::file_descriptor& in);

} // namespace img
