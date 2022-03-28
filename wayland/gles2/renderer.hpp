#pragma once

#include <cassert>
#include <chrono>
#include <span>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <wayland/gles2/gl_resource.hpp>
#include <wayland/util/geom.hpp>

struct vertex;

class mesh {
public:
  constexpr mesh() noexcept = default;
  explicit mesh(attrib_location<glm::vec3> pos, attrib_location<glm::vec3> norm,
                std::span<const vertex> verticies,
                std::span<const GLuint> indexes);

  void draw();

  constexpr explicit operator bool() const noexcept { return vbo_ && ibo_; };

private:
  attrib_location<glm::vec3> pos_;
  attrib_location<glm::vec3> norm_;
  buffer ibo_;
  buffer vbo_;
  unsigned triangles_count_ = 0;
};

class renderer {
  using clock = std::chrono::steady_clock;

public:
  renderer();

  void resize(size sz);
  void draw(clock::time_point ts);

  void camera_look_at(glm::vec3 eye, glm::vec3 center);

private:
  shader_program shader_prog_;
  uniform_location<glm::mat4> camera_uniform_;
  uniform_location<glm::mat4> model_world_uniform_;
  uniform_location<glm::mat3> norm_world_uniform_;
  mesh cube_;
  mesh landscape_;
  glm::mat4 projection_{};
  glm::mat4 camera_{};
};
