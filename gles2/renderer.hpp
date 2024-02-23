#pragma once

#include <cassert>
#include <chrono>
#include <span>

#include <util/clock.hpp>
#include <util/geom.hpp>

#include <gles2/shader_pipeline.hpp>

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
};
