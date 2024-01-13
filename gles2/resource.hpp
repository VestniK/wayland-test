#pragma once

#include <span>
#include <string_view>
#include <utility>

#include <GLES2/gl2.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <gles2/attrib.hpp>
#include <gles2/resource_handle.hpp>
#include <gles2/uniform.hpp>

namespace gl {

enum class handle_type { shader, shader_program, buffer, texture };

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
shader compile(shader_type type, std::span<const char* const> srcs);
shader compile(shader_type type, std::string_view src);

// shader program
using shader_program_handle = basic_handle<handle_type::shader_program>;
template <>
inline void shader_program_handle::free(shader_program_handle handle) noexcept {
  glDeleteProgram(handle.handle_);
}
resource<shader_program_handle> link(
    shader_handle vertex, shader_handle fragment);

class shader_program : public resource<shader_program_handle> {
public:
  explicit shader_program(shader_handle vertex, shader_handle fragment);
  explicit shader_program(
      const char* vertex_shader_sources, const char* fragment_shader_sources);
  explicit shader_program(std::span<const char* const> vertex_shader_sources,
      std::span<const char* const> fragment_shader_sources);
  explicit shader_program(std::string_view vertex_shader_sources,
      std::string_view fragment_shader_sources);

  void use() { glUseProgram(native_handle()); }

  template <typename T>
  uniform_location<T> get_uniform(const char* name) {
    return uniform_location<T>{glGetUniformLocation(native_handle(), name)};
  }

  template <typename T>
  attrib_location<T> get_attrib(const char* name) noexcept {
    return attrib_location<T>{glGetAttribLocation(native_handle(), name)};
  }
};

} // namespace gl
