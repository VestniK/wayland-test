#pragma once

#include <cassert>
#include <chrono>

#include <gsl/span>
#include <gsl/string_span>

#include <glm/glm.hpp>

#include <wayland/gles2/gl_resource.hpp>
#include <wayland/util/geom.hpp>

template <typename C, typename M>
using member_ptr = M C::*;

class shader_pipeline {
public:
  explicit shader_pipeline(gsl::span<gsl::czstring<>> vertex_shader_sources,
      gsl::span<gsl::czstring<>> fragment_shader_sources);

  void use();

  void set_uniform(gsl::czstring<> name, float value);
  void set_uniform(gsl::czstring<> name, const glm::mat4& value);
  void set_uniform(gsl::czstring<> name, const glm::mat3& value);
  void set_uniform(gsl::czstring<> name, const glm::vec3& value);

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
  mesh cube_;
  size sz_;
};
