#pragma once

#include <chrono>

#include <gsl/string_span>

#include <GLES2/gl2.h>

#include "geom.h"

class shader {
public:
  using native_handle_type = GLuint;
  enum class type: GLenum {
    vertex = GL_VERTEX_SHADER,
    fragment = GL_FRAGMENT_SHADER
  };

  shader() noexcept = default;

  shader(type type, gsl::czstring<> src);

  ~shader();

  shader(const shader&) = delete;
  shader& operator= (const shader&) = delete;

  shader(shader&& rhs) noexcept;
  shader& operator= (shader&& rhs) noexcept;

  native_handle_type native_handle() const noexcept {return shader_;}

  explicit operator bool () const noexcept {return shader_ != 0;}

private:
  GLuint shader_ = 0;
};

class elements_array_buffer {
public:
  using native_handle_type = GLuint;
  native_handle_type native_handle() const {return handle_;}

  elements_array_buffer();
  ~elements_array_buffer();

private:
  GLuint handle_ = 0;
};

class renderer {
  using clock = std::chrono::steady_clock;
public:
  renderer();
  ~renderer();

  void resize(size sz);
  void draw(clock::time_point ts);

private:
  shader vertex_shader_;
  shader fragment_shader_;
  GLuint program_ = 0;
  elements_array_buffer idxs_;
  int32_t len_;
};
