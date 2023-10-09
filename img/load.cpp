#include "load.hpp"

#include <spdlog/spdlog.h>

#include <gsl/narrow>

#include <png.h>

#include <util/geom.hpp>

namespace {
namespace png {

constexpr size_t signature_size = 8;

void warn_fn(png_struct*, const char* msg) { spdlog::warn("PNG: {}", msg); }

[[noreturn]] void err_fn(png_struct*, const char* msg) {
  throw std::runtime_error{msg};
}

void read_fn(png_struct* png_ptr, png_byte* data, size_t length) {
  auto& in = *static_cast<io::file_descriptor*>(png_get_io_ptr(png_ptr));
  while (length > 0) {
    const auto sz = read(in, std::as_writable_bytes(std::span{data, length}));
    if (sz == 0)
      throw std::runtime_error{"premature png file end"};
    length -= sz;
  }
}

struct read_info_struct_deleter {
  void operator()(png_structp ptr) noexcept {
    png_destroy_read_struct(&ptr, &info, nullptr);
  }

  png_info* info = nullptr;
};

using read_info_ptr = std::unique_ptr<png_struct, read_info_struct_deleter>;

} // namespace png
} // namespace

namespace img {

image load(io::file_descriptor& in) {
  std::array<std::byte, png::signature_size> sig;
  const auto read = io::read(in, sig);
  if (read != sig.size())
    throw std::runtime_error{"invalid png stream, premature end of file"};
  if (png_sig_cmp(
          reinterpret_cast<png_const_bytep>(sig.data()), 0, sig.size()) != 0)
    throw std::runtime_error{
        "invalid png stream, format signature verification failed"};

  png::read_info_ptr png_struct{png_create_read_struct(
      PNG_LIBPNG_VER_STRING, nullptr, &png::err_fn, &png::warn_fn)};
  png_struct.get_deleter().info = png_create_info_struct(png_struct.get());
  png_set_read_fn(png_struct.get(), &in, &png::read_fn);
  png_set_sig_bytes(png_struct.get(), png::signature_size);

  png_read_info(png_struct.get(), png_struct.get_deleter().info);

  const size img_size{gsl::narrow<int>(png_get_image_width(
                          png_struct.get(), png_struct.get_deleter().info)),
      gsl::narrow<int>(png_get_image_height(
          png_struct.get(), png_struct.get_deleter().info))};

  const auto bits =
      png_get_bit_depth(png_struct.get(), png_struct.get_deleter().info);
  const auto color =
      png_get_color_type(png_struct.get(), png_struct.get_deleter().info);
  const auto interlace_type =
      png_get_interlace_type(png_struct.get(), png_struct.get_deleter().info);

  if (color != PNG_COLOR_TYPE_RGB || bits != 8 ||
      interlace_type != PNG_INTERLACE_NONE)
    throw std::runtime_error{"Only non interlaced 8bit per channel RGB without "
                             "Alpha PNG images are supportted"};

  const size_t row_sz = img_size.width * 3;
  std::unique_ptr<std::byte[]> res{new std::byte[img_size.height * row_sz]};
  for (int row = 0; row < img_size.height; ++row) {
    png_read_row(png_struct.get(),
        reinterpret_cast<png_bytep>(res.get() + row * row_sz), nullptr);
  }

  return image{std::move(res), img_size};
}

} // namespace img
