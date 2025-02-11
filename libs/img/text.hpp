#pragma once

#include <memory>

#include <thinsys/io/io.hpp>

#include <libs/img/reader.hpp>

namespace img {

class font {
public:
  font() noexcept = default;

  font(const font&) = delete;
  font& operator=(const font&) = delete;

  font(font&&) noexcept = default;
  font& operator=(font&&) noexcept = default;

  ~font() noexcept;

  static font load(thinsys::io::file_descriptor& fd);

  reader text_image_reader(std::string_view text);

private:
  class impl;

  std::unique_ptr<impl> stm_;
};

} // namespace img
