#pragma once

#include <memory>
#include <span>

#include <thinsys/io/io.hpp>

#include <libs/geom/geom.hpp>
#include <libs/img/pixel_fmt.hpp>
#include <libs/img/reader.hpp>

namespace img {

reader load_reader(thinsys::io::file_descriptor& in);

template <pixel_fmt Fmt>
class image {
public:
  image() noexcept = default;
  image(std::unique_ptr<std::byte[]> data, ::size sz) noexcept : sz_{sz}, data_{std::move(data)} {}

  std::span<const std::byte> bytes() const noexcept { return {data_.get(), bytes_size(sz_, Fmt)}; }

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
