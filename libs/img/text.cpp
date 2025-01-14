#include "text.hpp"

#include <system_error>

#include <freetype/freetype.h>
#include <freetype/fterrors.h>

namespace img::detail::ft {

void deleter::operator()(FT_LibraryRec_* ptr) const noexcept { FT_Done_FreeType(ptr); }
void deleter::operator()(FT_FaceRec_* ptr) const noexcept { FT_Done_Face(ptr); }

namespace {

struct : std::error_category {
  const char* name() const noexcept override { return "freetype erros"; }
  std::string message(FT_Error ec) const override { return FT_Error_String(ec); }
} category;

unsigned long fd_io(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count) {
  // TODO !!!
  return 0;
}

} // namespace

} // namespace img::detail::ft

namespace img {

font font::load(thinsys::io::file_descriptor& fd) {
  font res;
  FT_Library lib;
  if (const int ec = FT_Init_FreeType(&lib); ec != 0)
    throw std::system_error{ec, detail::ft::category, "FT_Init_FreeType"};
  res.lib_.reset(lib);

  FT_StreamRec_ fd_stream{
      .base = nullptr,
      .size = 0x7FFFFFFF,
      .pos = 0,
      .descriptor = {.value = fd.native_handle()},
      .pathname = {.value = 0},
      .read = detail::ft::fd_io,
      .close = [](FT_Stream) {},
      .memory = {},
      .cursor = nullptr,
      .limit = nullptr
  };

  FT_Open_Args args{
      .flags = {},
      .memory_base = {},
      .memory_size = {},
      .pathname = {},
      .stream = &fd_stream,
      .driver = {},
      .num_params = {},
      .params = {},
  };
  FT_Face face;
  if (const FT_Error ec = FT_Open_Face(res.lib_.get(), &args, 0, &face); ec != 0)
    throw std::system_error{ec, detail::ft::category, "FT_Open_Face"};
  res.font_.reset(face);
  return res;
}

reader font::text_image_reader(std::string_view text) {
  // TODO !!!
  return {};
}

} // namespace img
