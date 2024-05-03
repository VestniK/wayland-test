#pragma once

#include <cassert>
#include <chrono>
#include <span>

#include <glm/mat4x4.hpp>

#include <libs/geom/geom.hpp>

#include <scene/anime.hpp>
#include <wayland/clock.hpp>

#include <apps/colorcube/shader_pipeline.hpp>

namespace scene {
class controller;
}

class scene_renderer {
  using clock = frames_clock;

public:
  scene_renderer(const scene::controller& controller);

  void resize(size sz);
  void draw(clock::time_point ts);

private:
  shader_pipeline pipeline_;
  const scene::controller& controller_;

  mesh cube_;
  mesh landscape_;
  glm::mat4 camera_;

  linear_animation landscape_color_;
  linear_animation cube_color_;
  clamped_integrator cube_pos_integrator_{
      {.min = {-0.5, -1.}, .max = {7.5, 7.}}, {}, {}};
};
