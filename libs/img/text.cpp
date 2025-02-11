#include "text.hpp"

#include <system_error>

#include <freetype/freetype.h>
#include <freetype/fterrors.h>
#include <freetype/ftglyph.h>

#include <iconv.h>

#include <fmt/format.h>

namespace {

static_assert(std::is_pointer_v<iconv_t>);
struct iconv_deleter {
  void operator()(iconv_t ptr) const noexcept { iconv_close(ptr); }
};
using iconv_ptr = std::unique_ptr<std::remove_pointer_t<iconv_t>, iconv_deleter>;

std::u32string utf8_to_utf32(std::string_view text) {
  iconv_ptr conv{iconv_open("UTF32LE", "UTF8")};
  if (conv.get() == (iconv_t)-1) {
    conv.release();
    throw std::system_error{errno, std::system_category(), "iconv_open"};
  }
  std::u32string chars{};
  chars.resize(text.size());
  size_t insz = text.size(), outsz = std::as_writable_bytes(std::span{chars}).size();
  char* inbuf = const_cast<char*>(text.data());
  char* outbuf = reinterpret_cast<char*>(chars.data());
  if (iconv(conv.get(), &inbuf, &insz, &outbuf, &outsz) == static_cast<size_t>(-1))
    throw std::system_error{errno, std::system_category(), "iconv"};
  chars.resize(chars.size() - (outsz) / sizeof(char32_t));
  return chars;
}

} // namespace

namespace img::ft {
namespace {

struct deleter {
  void operator()(FT_Library ptr) const noexcept { FT_Done_FreeType(ptr); }
  void operator()(FT_Face ptr) const noexcept { FT_Done_Face(ptr); }
};

using library = std::unique_ptr<FT_LibraryRec_, deleter>;
using face = std::unique_ptr<FT_FaceRec_, deleter>;

struct : std::error_category {
  const char* name() const noexcept override { return "freetype error"; }
  std::string message(FT_Error ec) const override { return FT_Error_String(ec); }
} category;

library init() {
  FT_Library lib;
  if (const int ec = FT_Init_FreeType(&lib); ec != 0)
    throw std::system_error{ec, ft::category, "FT_Init_FreeType"};
  return library{lib};
}

class fd_stream {
public:
  fd_stream(thinsys::io::file_descriptor& fd)
      : fd_{fd}, start_{thinsys::io::seek(fd_, 0, thinsys::io::seek_whence::cur)},
        stream_{
            .base = nullptr,
            .size = 0x7FFFFFFF,
            .pos = 0,
            .descriptor = {.pointer = this},
            .pathname = {.value = 0},
            .read = fd_io,
            .close = noclose,
            .memory = {},
            .cursor = nullptr,
            .limit = nullptr
        } {}

  fd_stream(const fd_stream&) = delete;
  fd_stream& operator=(const fd_stream&) = delete;
  fd_stream(fd_stream&&) = delete;
  fd_stream& operator=(fd_stream&&) = delete;
  ~fd_stream() noexcept = default;

  face open_face(library::element_type& lib) {
    FT_Open_Args args{
        .flags = FT_OPEN_STREAM,
        .memory_base = {},
        .memory_size = {},
        .pathname = {},
        .stream = &stream_,
        .driver = {},
        .num_params = {},
        .params = {},
    };
    FT_Face res;
    if (const FT_Error ec = FT_Open_Face(&lib, &args, 0, &res); ec != 0)
      throw std::system_error{ec, ft::category, "FT_Open_Face"};
    return face{res};
  }

private:
  static unsigned long
  fd_io(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count) noexcept {
    const auto& self = *reinterpret_cast<class fd_stream*>(stream->descriptor.pointer);
    std::error_code ec;
    thinsys::io::seek(self.fd_, offset + self.start_, thinsys::io::seek_whence::set, ec);
    if (count == 0)
      return ec ? 1 : 0;

    if (ec)
      return 0;

    return thinsys::io::read(self.fd_, std::as_writable_bytes(std::span{buffer, count}), ec);
  }

  static void noclose(FT_Stream) noexcept {}

private:
  thinsys::io::file_descriptor& fd_;
  std::streampos start_;
  FT_StreamRec_ stream_;
};

} // namespace
} // namespace img::ft

namespace img {

class font::impl {
public:
  impl(thinsys::io::file_descriptor& fd) : lib{ft::init()}, stream{fd}, font{stream.open_face(*lib)} {
    if (FT_Error ec = FT_Select_Charmap(font.get(), ft_encoding_unicode); ec != 0)
      throw std::system_error{ec, ft::category, "FT_Select_Charmap"};
  }

  void set_pixel_size(FT_UInt width, FT_UInt height) {
    if (FT_Error ec = FT_Set_Pixel_Sizes(font.get(), width, height); ec != 0)
      throw std::system_error{ec, ft::category, "FT_Set_Pixel_Sizes"};
  }

  ft::library lib;
  ft::fd_stream stream;
  ft::face font;
};

font::~font() noexcept = default;

font font::load(thinsys::io::file_descriptor& fd) {
  font res;
  res.stm_ = std::make_unique<impl>(fd);
  res.stm_->set_pixel_size(128, 128);

  return res;
}

reader font::text_image_reader(std::string_view text) {
  auto chars = utf8_to_utf32(text);

  int top = 0;
  int bottom = 0;
  FT_Pos width = 0;
  for (char32_t ch : chars) {
    const auto idx = FT_Get_Char_Index(stm_->font.get(), ch);
    if (FT_Error ec = FT_Load_Glyph(stm_->font.get(), idx, FT_LOAD_BITMAP_METRICS_ONLY); ec != 0)
      throw std::system_error{ec, ft::category, "FT_Load_Glyph"};
    top = std::max(top, stm_->font->glyph->bitmap_top);
    bottom =
        std::min(bottom, stm_->font->glyph->bitmap_top - static_cast<int>(stm_->font->glyph->bitmap.rows));
    width += stm_->font->glyph->advance.x >> 6;
  }
  width += 2;
  return {
      size{static_cast<int32_t>(width), static_cast<int32_t>(top - bottom + 2)}, pixel_fmt::rgba,
      [chars = std::move(chars), font = stm_->font.get(), top, width](std::span<std::byte> dest) -> size_t {
        std::ranges::fill(dest, std::byte{0});

        FT_Pos left = 1;
        for (char32_t ch : chars) {
          const auto idx = FT_Get_Char_Index(font, ch);
          if (FT_Error ec = FT_Load_Glyph(font, idx, FT_LOAD_DEFAULT); ec != 0)
            throw std::system_error{ec, ft::category, "FT_Load_Glyph"};
          if (FT_Error ec = FT_Render_Glyph(font->glyph, FT_RENDER_MODE_NORMAL); ec != 0)
            throw std::system_error{ec, ft::category, "FT_Render_Glyph"};

          const std::byte* bitmap_pos = reinterpret_cast<const std::byte*>(font->glyph->bitmap.buffer);
          for (auto row = 0; row < font->glyph->bitmap.rows; ++row) {
            const auto y = top - font->glyph->bitmap_top + row + 1;
            const auto x = left + font->glyph->bitmap_left;
            auto* dest_pos = dest.data() + (y * width + x) * pixel_byte_size(pixel_fmt::rgba);

            for (std::byte val : std::span{bitmap_pos, static_cast<size_t>(font->glyph->bitmap.width)}) {
              *(dest_pos++) = std::byte{0x7a};
              *(dest_pos++) = std::byte{0x7a};
              *(dest_pos++) = std::byte{0x7a};
              *(dest_pos++) = val;
            }

            bitmap_pos += font->glyph->bitmap.pitch;
          }

          left += font->glyph->advance.x >> 6;
        }

        return 0;
      }
  };
}

} // namespace img
