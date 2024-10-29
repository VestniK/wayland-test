#pragma once

#include <functional>
#include <memory>
#include <span>

#include <libs/geom/geom.hpp>
#include <thinsys/io/io.hpp>

namespace img {

enum class pixel_fmt : size_t { rgb = 3, rgba = 4 };

class reader {
public:
  reader() noexcept = default;
  reader(::size sz, pixel_fmt fmt,
      std::move_only_function<size_t(std::span<std::byte>)> read_pixels)
      : read_pixels_{std::move(read_pixels)}, sz_{sz}, fmt_{fmt} {}

  ::size size() const noexcept { return sz_; }
  pixel_fmt format() const noexcept { return fmt_; }

  size_t pixels_size() const noexcept {
    return sz_.height * sz_.width * std::to_underlying(fmt_);
  }

  size_t read_pixels(std::span<std::byte> dest) { return read_pixels_(dest); }

private:
  std::move_only_function<size_t(std::span<std::byte>)> read_pixels_;
  ::size sz_;
  pixel_fmt fmt_;
};

reader load_reader(thinsys::io::file_descriptor& in);

template <pixel_fmt Fmt>
class image {
public:
  image() noexcept = default;
  image(std::unique_ptr<std::byte[]> data, ::size sz) noexcept
      : sz_{sz}, data_{std::move(data)} {}

  std::span<const std::byte> bytes() const noexcept {
    return {data_.get(), sz_.height * sz_.width * std::to_underlying(Fmt)};
  }

  ::size size() const noexcept { return sz_; }

private:
  ::size sz_;
  std::unique_ptr<std::byte[]> data_;
};

template <pixel_fmt Fmt>
image<Fmt> load(thinsys::io::file_descriptor& in) {
  auto source = load_reader(in);
  if (source.format() != Fmt)
    throw std::runtime_error{"Unexpected image format"};

  std::unique_ptr<std::byte[]> res{new std::byte[source.pixels_size()]};
  source.read_pixels({res.get(), source.pixels_size()});
  return image<Fmt>{std::move(res), source.size()};
}

} // namespace img
