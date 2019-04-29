#pragma once

#include <cassert>
#include <chrono>

#include <gsl/span>
#include <gsl/string_span>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <wayland/gles2/gl_resource.hpp>
#include <wayland/util/geom.hpp>

template <typename T>
class uniform_location {
public:
  constexpr uniform_location() noexcept = default;
  constexpr explicit uniform_location(GLint location) noexcept
      : location_{location} {}

  void set_value(const T& val) = delete;

private:
  GLint location_ = 0;
};

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

template <typename C, typename M>
using member_ptr = M C::*;

class shader_pipeline {
public:
  explicit shader_pipeline(gsl::span<gsl::czstring<>> vertex_shader_sources,
      gsl::span<gsl::czstring<>> fragment_shader_sources);

  void use();

  template <typename T>
  uniform_location<T> get_uniform(gsl::czstring<> name) {
    return uniform_location<T>{glGetUniformLocation(program_.get(), name)};
  }

  template <typename C>
  void set_attrib_pointer(
      gsl::czstring<> name, member_ptr<C, glm::vec3> member) {
    const GLint id = glGetAttribLocation(program_.get(), name);
    glVertexAttribPointer(id, 3, GL_FLOAT, GL_FALSE, sizeof(C),
        static_cast<const GLvoid*>(&(static_cast<const C*>(nullptr)->*member)));
    glEnableVertexAttribArray(id);
  }

private:
  shader_program program_;
};

struct vertex {
  glm::vec3 position;
  glm::vec3 normal;
};

class mesh {
public:
  explicit mesh(
      gsl::span<const vertex> verticies, gsl::span<const GLuint> indexes);

  void draw(shader_pipeline& pipeline, gsl::czstring<> pos_name,
      gsl::czstring<> normal_name);

private:
  buffer ibo_;
  buffer vbo_;
  unsigned triangles_count_;
};

class renderer {
  using clock = std::chrono::steady_clock;

public:
  renderer();

  void resize(size sz);
  void draw(clock::time_point ts);

private:
  shader_pipeline pipeline_;
  uniform_location<glm::mat4> camera_uniform_;
  uniform_location<glm::vec3> light_pos_uniform_;
  uniform_location<glm::mat4> model_world_uniform_;
  uniform_location<glm::mat3> norm_world_uniform_;
  mesh cube_;
  size sz_;
};
