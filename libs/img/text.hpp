#pragma once

#include <memory>

#include <thinsys/io/io.hpp>

#include <libs/img/reader.hpp>

struct FT_LibraryRec_;
struct FT_FaceRec_;

namespace img {

namespace detail::ft {

struct deleter {
  void operator()(FT_LibraryRec_* ptr) const noexcept;
  void operator()(FT_FaceRec_* ptr) const noexcept;
};

using library = std::unique_ptr<FT_LibraryRec_, deleter>;
using face = std::unique_ptr<FT_FaceRec_, deleter>;

} // namespace detail::ft

class font {
public:
  static font load(thinsys::io::file_descriptor& fd);

  reader text_image_reader(std::string_view text);

private:
  detail::ft::library lib_;
  detail::ft::face font_;
};

} // namespace img
