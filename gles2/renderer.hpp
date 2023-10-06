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

struct animate_to {
  glm::vec2 dest;
  std::chrono::milliseconds duration;

  bool operator==(const animate_to&) const noexcept = default;
};
struct linear_animation {
  glm::vec2 start;
  frames_clock::time_point start_time;
  glm::vec2 end;
  frames_clock::time_point end_time;

  void reset(frames_clock::time_point now, animate_to animation) noexcept {
    start = animate(now);
    start_time = now;
    end = animation.dest;
    end_time = now + animation.duration;
  }

  glm::vec2 animate(frames_clock::time_point now) const noexcept {
    if (now >= end_time)
      return end;
    const auto factor = float_time::milliseconds{now - start_time} /
                        float_time::milliseconds{end_time - start_time};
    return start + (end - start) * factor;
  }
};

class scene_renderer {
  using clock = frames_clock;

public:
  class controller;

  scene_renderer(
      img::image cube_tex, img::image land_tex, const controller& controller);

  void resize(size sz);
  void draw(clock::time_point ts);

private:
  shader_pipeline pipeline_;
  mesh cube_;
  mesh landscape_;
  glm::mat4 projection_{};

  texture cube_tex_;
  texture land_tex_;

  const controller& controller_;

  linear_animation cube_tex_offset_;
  struct {
    glm::vec2 cur_pos{};
    clock::time_point last_ts{};
  } cube_planar_movement_{};
};

class scene_renderer::controller {
public:
  glm::ivec2 current_cube_vel() const noexcept {
    return cube_vel_.get_current();
  }

  std::optional<animate_to> cube_tex_offset_update() const noexcept {
    return cube_tex_offset_update_.get_update();
  }

protected:
  value_update_channel<animate_to> cube_tex_offset_update_;
  value_update_channel<glm::ivec2> cube_vel_;
};
