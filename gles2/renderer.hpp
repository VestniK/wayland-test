#pragma once

#include <cassert>
#include <chrono>
#include <span>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <util/clock.hpp>
#include <util/geom.hpp>

#include <gles2/gl_resource.hpp>

struct vertex;
class mesh;

class shader_pipeline {
public:
  struct attributes {
    attrib_location<glm::vec3> position;
    attrib_location<glm::vec3> normal;
  };

  shader_pipeline();

  void start_rendering(glm::mat4 camera);
  void draw(glm::mat4 model, glm::vec3 color, mesh &mesh);

private:
  shader_program shader_prog_;
  uniform_location<glm::mat4> camera_uniform_;
  uniform_location<glm::mat4> model_world_uniform_;
  uniform_location<glm::mat3> norm_world_uniform_;
  uniform_location<glm::vec3> color_uniform_;
  attributes attributes_;
};

class mesh {
public:
  constexpr mesh() noexcept = default;
  explicit mesh(std::span<const vertex> verticies,
                std::span<const GLuint> indexes);

  void draw(shader_pipeline::attributes attrs);

  constexpr explicit operator bool() const noexcept { return vbo_ && ibo_; };

private:
  buffer ibo_;
  buffer vbo_;
  unsigned triangles_count_ = 0;
};

class scene_renderer {
  using clock = frames_clock;

public:
  scene_renderer();

  void resize(size sz);
  void draw(clock::time_point ts);

private:
  shader_pipeline pipeline_;
  mesh cube_;
  mesh landscape_;
  glm::mat4 projection_{};
};
