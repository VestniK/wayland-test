#pragma once

#include <cassert>
#include <chrono>

#include <gsl/string_span>

#include <wayland/gles2/gl_resource.hpp>
#include <wayland/util/geom.hpp>

class renderer {
  using clock = std::chrono::steady_clock;

public:
  renderer();

  void resize(size sz);
  void draw(clock::time_point ts);

private:
  shader vertex_shader_;
  shader fragment_shader_;
  program program_;
  buffer ibo_;
  buffer vbo_;
  size sz_;
};
