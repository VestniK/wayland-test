#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>

#include <GLES2/gl2.h>

#include <util/geom.hpp>

#include <libs/gles2/resource.hpp>
#include <libs/gles2/uniform.hpp>

namespace gl {

enum class texel_format { rgb = GL_RGB, rgba = GL_RGBA };

class texture_sampler {
public:
  struct bind_editor {
    bind_editor& image_2d(
        texel_format fmt, ::size sz, std::span<const std::byte> img) noexcept {
      glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(fmt), sz.width,
          sz.height, 0, static_cast<GLint>(fmt), GL_UNSIGNED_BYTE, img.data());
      return *this;
    }

    bind_editor& tex_parameter(GLenum tgt, GLint val) noexcept {
      glTexParameteri(GL_TEXTURE_2D, tgt, val);
      return *this;
    }
  };

public:
  constexpr texture_sampler(GLint id = GL_TEXTURE0) noexcept : id_{id} {}

  constexpr GLint native_handle() const noexcept { return id_; }

  bind_editor bind(texture_handle id) {
    glActiveTexture(id_);
    glBindTexture(GL_TEXTURE_2D, id.native_handle());
    return bind_editor{};
  }

private:
  GLint id_;
};

struct texture_samplers {
  static constexpr size_t min_size = 8;

  static size_t size() noexcept {
    GLint val;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &val);
    return static_cast<size_t>(val);
  }

  static constexpr texture_sampler operator[](size_t idx) noexcept {
    if (std::is_constant_evaluated())
      assert(idx < min_size);
    else
      assert(idx < size());
    return texture_sampler{GL_TEXTURE0 + static_cast<GLint>(idx)};
  }
};
inline constexpr texture_samplers samplers = {};

template <>
inline void uniform_location<texture_sampler>::set_value(
    const texture_sampler& val) {
  glUniform1i(location_, val.native_handle() - GL_TEXTURE0);
}

} // namespace gl
