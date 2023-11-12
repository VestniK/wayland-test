#pragma once

#include <cassert>
#include <chrono>
#include <span>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <util/channel.hpp>
#include <util/clock.hpp>
#include <util/geom.hpp>

#include <img/load.hpp>

#include <scene/controller.hpp>

#include <gles2/gl_resource.hpp>

struct vertex;
class mesh;

class shader_pipeline {
public:
  struct attributes {
    attrib_location<glm::vec3> position;
    attrib_location<glm::vec3> normal;
    attrib_location<glm::vec2> uv;
  };

  shader_pipeline();

  void start_rendering(glm::mat4 camera);
  void draw(
      glm::mat4 model, int tex_idx, mesh& mesh, glm::vec2 tex_offset = {});

private:
  shader_program shader_prog_;
  uniform_location<glm::mat4> camera_uniform_;
  uniform_location<glm::mat4> model_world_uniform_;
  uniform_location<glm::mat3> norm_world_uniform_;
  uniform_location<glm::vec2> tex_offset_uniform_;
  uniform_location<GLint> texture_index_uniform_;
  attributes attributes_;
};

class mesh {
public:
  constexpr mesh() noexcept = default;
  explicit mesh(
      std::span<const vertex> verticies, std::span<const GLuint> indexes);

  void draw(shader_pipeline::attributes attrs);

  constexpr explicit operator bool() const noexcept { return vbo_ && ibo_; };

private:
  buffer ibo_;
  buffer vbo_;
  unsigned triangles_count_ = 0;
};

struct bounding_box {
  glm::vec2 min = glm::vec2{std::numeric_limits<glm::vec2::value_type>::min(),
      std::numeric_limits<glm::vec2::value_type>::min()};
  glm::vec2 max = glm::vec2{std::numeric_limits<glm::vec2::value_type>::max(),
      std::numeric_limits<glm::vec2::value_type>::max()};
  ;

  glm::vec2 clamp(glm::vec2 pt) const noexcept {
    return {std::clamp(pt.x, min.x, max.x), std::clamp(pt.y, min.y, max.y)};
  }
};

class clamped_integrator {
public:
  clamped_integrator(bounding_box bounds, glm::vec2 start_val,
      frames_clock::time_point start_time) noexcept
      : bounds_{bounds}, current_{start_val}, last_ts_{start_time} {}
  glm::vec2 operator()(
      glm::vec2 velocity, frames_clock::time_point ts) noexcept {
    current_ += velocity * std::chrono::duration_cast<float_time::seconds>(
                               ts - std::exchange(last_ts_, ts))
                               .count();
    current_ = bounds_.clamp(current_);
    return current_;
  }

private:
  bounding_box bounds_;
  glm::vec2 current_;
  frames_clock::time_point last_ts_;
};

class scene_renderer {
  using clock = frames_clock;

public:
  scene_renderer(img::image cube_tex, img::image land_tex,
      const scene::controller& controller);

  void resize(size sz);
  void draw(clock::time_point ts);

private:
  shader_pipeline pipeline_;
  mesh cube_;
  mesh landscape_;
  glm::mat4 projection_{};

  texture cube_tex_;
  texture land_tex_;

  const scene::controller& controller_;

  linear_animation cube_tex_offset_;
  clamped_integrator cube_pos_integrator_{
      {.min = {-0.5, -1.}, .max = {7.5, 7.}}, {}, {}};
  clamped_integrator camera_center_integrator_{
      {.min = {0., 0.}, .max = {6., 4.}}, {3., 2.}, {}};
};
