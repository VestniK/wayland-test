#pragma once

#include <cassert>
#include <chrono>
#include <span>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <util/channel.hpp>
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
  void draw(glm::mat4 model, glm::vec3 color, mesh& mesh);

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
  glm::vec3 dest;
  std::chrono::milliseconds duration;

  bool operator==(const animate_to&) const noexcept = default;
};
struct linear_animation {
  glm::vec3 start;
  frames_clock::time_point start_time;
  glm::vec3 end;
  frames_clock::time_point end_time;

  void reset(frames_clock::time_point now, animate_to animation) noexcept {
    start = animate(now);
    start_time = now;
    end = animation.dest;
    end_time = now + animation.duration;
  }

  glm::vec3 animate(frames_clock::time_point now) const noexcept {
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
  scene_renderer(value_update_channel<animate_to>& cube_color_updates,
      value_update_channel<animate_to>& landscape_color_updates,
      value_update_channel<glm::ivec2>& cube_vel);

  void resize(size sz);
  void draw(clock::time_point ts);

private:
  shader_pipeline pipeline_;
  mesh cube_;
  mesh landscape_;
  glm::mat4 projection_{};

  value_update_channel<animate_to>& cube_color_updates_;
  linear_animation cube_color_;

  value_update_channel<animate_to>& landscape_color_updates_;
  linear_animation landscape_color_;

  value_update_channel<glm::ivec2>& cube_vel_;
  struct {
    glm::vec2 cur_pos{};
    clock::time_point last_ts{};
  } cube_planar_movement_{};
};
