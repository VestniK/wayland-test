#pragma once

#include <utility>

#include <GLES2/gl2.h>

#include <gsl/string_span>

#include <wayland/util/geom.hpp>

template <typename Deleter>
class gl_resource : Deleter {
public:
  constexpr gl_resource() noexcept = default;
  constexpr gl_resource(GLuint handle) noexcept : handle_{handle} {}

  ~gl_resource() {
    if (handle_ != 0)
      static_cast<Deleter&> (*this)(handle_);
  }

  gl_resource(const gl_resource&) = delete;
  gl_resource& operator=(const gl_resource&) = delete;

  constexpr gl_resource(gl_resource&& rhs) noexcept
      : handle_{std::exchange(rhs.handle_, 0)} {}
  constexpr gl_resource& operator=(gl_resource&& rhs) noexcept {
    gl_resource old{std::exchange(handle_, std::exchange(rhs.handle_, 0))};
    return *this;
  }

  constexpr GLuint get() const noexcept { return handle_; }
  constexpr GLuint release() noexcept { return std::exchange(handle_, 0); }

  constexpr explicit operator bool() const noexcept { return handle_ != 0; }

private:
  GLuint handle_ = 0;
};

// shader
struct shader_deleter {
  void operator()(GLuint handle) { glDeleteShader(handle); }
};
using shader = gl_resource<shader_deleter>;
enum class shader_type : GLenum {
  vertex = GL_VERTEX_SHADER,
  fragment = GL_FRAGMENT_SHADER
};
shader compile(shader_type type, gsl::czstring<> src);
shader compile(shader_type type, gsl::span<gsl::czstring<>> srcs);

// shader program
struct program_deleter {
  void operator()(GLuint handle) { glDeleteProgram(handle); }
};
using shader_program = gl_resource<program_deleter>;
shader_program link(const shader& vertex, const shader& fragment);

// Buffers
struct buffer_deleter {
  void operator()(GLuint handle) { glDeleteBuffers(1, &handle); }
};

using buffer = gl_resource<buffer_deleter>;
inline buffer gen_buffer() {
  GLuint handle;
  glGenBuffers(1, &handle);
  return {handle};
}
