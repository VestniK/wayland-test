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

  constexpr gl_resource(gl_resource&&) noexcept = default;
  constexpr gl_resource& operator=(gl_resource&&) noexcept = delete;

  constexpr GLuint get() const noexcept { return handle_; }
  constexpr GLuint release() noexcept { return std::exchange(handle_, 0); }

  constexpr explicit operator bool() noexcept { return handle_ != 0; }

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

// shader program
struct program_deleter {
  void operator()(GLuint handle) { glDeleteProgram(handle); }
};
using program = gl_resource<program_deleter>;
program link(const shader& vertex, const shader& fragment);

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
