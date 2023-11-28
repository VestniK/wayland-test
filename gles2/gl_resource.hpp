#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <span>
#include <utility>

#include <GLES2/gl2.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <util/member.hpp>

namespace gl {

template <typename T>
concept resource_handle =
    std::regular<T> && std::same_as<typename T::native_handle_t, GLuint> &&
    requires(const T& t) {
      { t.native_handle() } -> std::same_as<typename T::native_handle_t>;
      { static_cast<bool>(t) };
      { T::invalid } -> std::same_as<const T&>;
    };

template <typename T>
concept scalar_resource_handle = resource_handle<T> && requires(T t) {
  { T::free(t) };
};

template <typename T>
concept array_resource_handle = resource_handle<T> && requires(std::span<T> t) {
  { T::free(t) };
};

template <typename T>
class resource;

template <scalar_resource_handle T>
class resource<T> {
public:
  using native_handle_t = T::native_handle_t;

  constexpr resource() noexcept = default;
  constexpr resource(T handle) noexcept : handle_{handle} {}

  resource(const resource&) = delete;
  resource& operator=(const resource&) = delete;

  resource(resource&& rhs) noexcept : handle_(rhs.release()) {}
  resource& operator=(resource&& rhs) noexcept {
    if (auto old = std::exchange(handle_, rhs.release()))
      T::free(old);
    return *this;
  }

  ~resource() noexcept {
    if (handle_)
      T::free(handle_);
  }

  constexpr explicit operator bool() const noexcept {
    return static_cast<bool>(handle_);
  }
  constexpr T release() noexcept { return std::exchange(handle_, T::invalid); }
  constexpr T get() noexcept { return handle_; }
  constexpr native_handle_t native_handle() noexcept {
    return handle_.native_handle();
  }

private:
  T handle_ = T::invalid;
};

template <array_resource_handle T, size_t N>
class resource<T[N]> {
public:
  constexpr resource() noexcept { handls_.fill(T::invalid); }
  constexpr resource(std::span<const T, N> handls) noexcept {
    assert(std::ranges::all_of(handls, [](const T& h) { return !h; }) ||
           std::ranges::none_of(handls, [](const T& h) { return !h; }));
    std::ranges::copy(handls, handls_.begin());
  }

  resource(const resource&) = delete;
  resource& operator=(const resource&) = delete;

  resource(resource&& rhs) noexcept {
    auto oit = handls_.begin();
    for (auto& handle : rhs.handls_)
      *(oit++) = std::exchange(handle, T::invalid);
  }
  resource& operator=(resource&& rhs) noexcept {
    if (handls_.front())
      T::free(handls_);
    auto oit = handls_.begin();
    for (auto& handle : rhs.handls_)
      *(oit++) = std::exchange(handle, T::invalid);
    return *this;
  }

  ~resource() noexcept {
    if (handls_.front())
      T::free(handls_);
  }

  constexpr const T& operator[](size_t idx) const noexcept {
    return handls_[idx];
  }

private:
  std::array<T, N> handls_;
};

enum class handle_type { shader, shader_program, buffer, texture };

template <auto Type>
class basic_handle {
public:
  using native_handle_t = GLuint;

  constexpr basic_handle() noexcept = default;
  constexpr explicit basic_handle(native_handle_t handle) noexcept
      : handle_{handle} {}
  constexpr std::strong_ordering operator<=>(
      const basic_handle&) const noexcept = default;

  constexpr native_handle_t native_handle() const noexcept { return handle_; }
  constexpr explicit operator bool() const noexcept {
    return handle_ != invalid.handle_;
  }

  static void free(basic_handle res) noexcept = delete;
  static void free(std::span<basic_handle> res) noexcept = delete;
  static const basic_handle invalid;

private:
  GLuint handle_ = 0;
};
template <auto Type>
constexpr basic_handle<Type> basic_handle<Type>::invalid = {};

// shader
using shader_handle = basic_handle<handle_type::shader>;
template <>
inline void shader_handle::free(shader_handle handle) noexcept {
  glDeleteShader(handle.handle_);
}
using shader = resource<shader_handle>;
enum class shader_type : GLenum {
  vertex = GL_VERTEX_SHADER,
  fragment = GL_FRAGMENT_SHADER
};
shader compile(shader_type type, const char* src);
shader compile(shader_type type, std::span<const char*> srcs);

// shader program
using shader_program_handle = basic_handle<handle_type::shader_program>;
template <>
inline void shader_program_handle::free(shader_program_handle handle) noexcept {
  glDeleteProgram(handle.handle_);
}
using shader_program = resource<shader_program_handle>;
shader_program link(shader_handle vertex, shader_handle fragment);

// buffers
using buffer_handle = basic_handle<handle_type::buffer>;
template <>
inline void buffer_handle::free(std::span<buffer_handle> handls) noexcept {
  glDeleteBuffers(handls.size(), reinterpret_cast<GLuint*>(handls.data()));
}

template <size_t N>
using buffers = resource<buffer_handle[N]>;
template <size_t N>
inline buffers<N> gen_buffers() {
  buffer_handle handls[N];
  glGenBuffers(N, reinterpret_cast<GLuint*>(handls));
  return {handls};
}

// textures
using texture_handle = basic_handle<handle_type::texture>;
template <>
inline void texture_handle::free(std::span<texture_handle> handls) noexcept {
  glDeleteTextures(handls.size(), reinterpret_cast<GLuint*>(handls.data()));
}

template <size_t N>
using textures = resource<texture_handle[N]>;
template <size_t N>
inline textures<N> gen_textures() {
  texture_handle handls[N];
  glGenTextures(N, reinterpret_cast<GLuint*>(handls));
  return {handls};
}

} // namespace gl

// shadet program attributes
template <typename T>
class attrib_location {
public:
  constexpr attrib_location() noexcept = default;
  constexpr explicit attrib_location(GLint location) noexcept
      : location_{location} {}

  template <typename C>
  void set_pointer(member_ptr<C, T> member);

private:
  GLint location_ = 0;
};

template <>
template <typename C>
void attrib_location<glm::vec3>::set_pointer(member_ptr<C, glm::vec3> member) {
  glVertexAttribPointer(location_, 3, GL_FLOAT, GL_FALSE, sizeof(C),
      reinterpret_cast<const GLvoid*>(member_offset(member)));
  glEnableVertexAttribArray(location_);
}

template <>
template <typename C>
void attrib_location<glm::vec2>::set_pointer(member_ptr<C, glm::vec2> member) {
  glVertexAttribPointer(location_, 2, GL_FLOAT, GL_FALSE, sizeof(C),
      reinterpret_cast<const GLvoid*>(member_offset(member)));
  glEnableVertexAttribArray(location_);
}

// shader program uniforms
template <typename T>
class uniform_location {
public:
  constexpr uniform_location() noexcept = default;
  constexpr explicit uniform_location(GLint location) noexcept
      : location_{location} {}

  // must be specialized for each reauired T
  void set_value(const T& val) = delete;

private:
  GLint location_ = 0;
};

template <>
inline void uniform_location<GLint>::set_value(const GLint& val) {
  glUniform1i(location_, val);
}

template <>
inline void uniform_location<float>::set_value(const float& val) {
  glUniform1f(location_, val);
}

template <>
inline void uniform_location<glm::mat4>::set_value(const glm::mat4& val) {
  glUniformMatrix4fv(location_, 1, GL_FALSE, glm::value_ptr(val));
}

template <>
inline void uniform_location<glm::mat3>::set_value(const glm::mat3& val) {
  glUniformMatrix3fv(location_, 1, GL_FALSE, glm::value_ptr(val));
}

template <>
inline void uniform_location<glm::vec3>::set_value(const glm::vec3& val) {
  glUniform3fv(location_, 1, glm::value_ptr(val));
}

template <>
inline void uniform_location<glm::vec2>::set_value(const glm::vec2& val) {
  glUniform2fv(location_, 1, glm::value_ptr(val));
}

// shader program
class shader_program {
public:
  explicit shader_program(
      const char* vertex_shader_sources, const char* fragment_shader_sources);

  void use();

  template <typename T>
  uniform_location<T> get_uniform(const char* name) {
    return uniform_location<T>{
        glGetUniformLocation(program_.native_handle(), name)};
  }

  template <typename T>
  attrib_location<T> get_attrib(const char* name) noexcept {
    return attrib_location<T>{
        glGetAttribLocation(program_.native_handle(), name)};
  }

private:
  gl::shader_program program_;
};
