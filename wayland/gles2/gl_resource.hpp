#pragma once

#include <span>
#include <utility>

#include <GLES2/gl2.h>

#include <gsl/string_span>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <wayland/util/member.hpp>

template <typename Deleter> class gl_resource : Deleter {
public:
  constexpr gl_resource() noexcept = default;
  constexpr gl_resource(GLuint handle) noexcept : handle_{handle} {}

  ~gl_resource() {
    if (handle_ != 0)
      static_cast<Deleter &> (*this)(handle_);
  }

  gl_resource(const gl_resource &) = delete;
  gl_resource &operator=(const gl_resource &) = delete;

  constexpr gl_resource(gl_resource &&rhs) noexcept
      : handle_{std::exchange(rhs.handle_, 0)} {}
  constexpr gl_resource &operator=(gl_resource &&rhs) noexcept {
    gl_resource old{std::exchange(handle_, std::exchange(rhs.handle_, 0))};
    return *this;
  }

  [[nodiscard]] constexpr GLuint get() const noexcept { return handle_; }
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
shader compile(shader_type type, std::span<const gsl::czstring<>> srcs);

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

// shadet program attributes
template <typename T> class attrib_location {
public:
  constexpr attrib_location() noexcept = default;
  constexpr explicit attrib_location(GLint location) noexcept
      : location_{location} {}

  template <typename C> void set_pointer(member_ptr<C, T> member);

private:
  GLint location_ = 0;
};

template <>
template <typename C>
void attrib_location<glm::vec3>::set_pointer(member_ptr<C, glm::vec3> member) {
  glVertexAttribPointer(
      location_, 3, GL_FLOAT, GL_FALSE, sizeof(C),
      reinterpret_cast<const GLvoid *>(member_offset(member)));
  glEnableVertexAttribArray(location_);
}

// shader program uniforms
template <typename T> class uniform_location {
public:
  constexpr uniform_location() noexcept = default;
  constexpr explicit uniform_location(GLint location) noexcept
      : location_{location} {}

  // must be specialized for each reauired T
  void set_value(const T &val) = delete;

private:
  GLint location_ = 0;
};

template <> inline void uniform_location<float>::set_value(const float &val) {
  glUniform1f(location_, val);
}

template <>
inline void uniform_location<glm::mat4>::set_value(const glm::mat4 &val) {
  glUniformMatrix4fv(location_, 1, GL_FALSE, glm::value_ptr(val));
}

template <>
inline void uniform_location<glm::mat3>::set_value(const glm::mat3 &val) {
  glUniformMatrix3fv(location_, 1, GL_FALSE, glm::value_ptr(val));
}

template <>
inline void uniform_location<glm::vec3>::set_value(const glm::vec3 &val) {
  glUniform3fv(location_, 1, glm::value_ptr(val));
}

// shader program
struct program_deleter {
  void operator()(GLuint handle) { glDeleteProgram(handle); }
};
gl_resource<program_deleter> link(const shader &vertex, const shader &fragment);

class shader_program {
public:
  explicit shader_program(gsl::czstring<> vertex_shader_sources,
                          gsl::czstring<> fragment_shader_sources);

  void use();

  template <typename T> uniform_location<T> get_uniform(gsl::czstring<> name) {
    return uniform_location<T>{
        glGetUniformLocation(program_handle_.get(), name)};
  }

  template <typename T>
  attrib_location<T> get_attrib(gsl::czstring<> name) noexcept {
    return attrib_location<T>{glGetAttribLocation(program_handle_.get(), name)};
  }

private:
  gl_resource<program_deleter> program_handle_;
};
