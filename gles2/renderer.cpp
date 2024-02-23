#include <vector>

#include <Tracy/tracy/Tracy.hpp>

#include <gles2/renderer.hpp>
#include <gles2/shaders.hpp>

using namespace std::literals;

scene_renderer::scene_renderer(const scene::controller& contr)
    : controller_{contr} {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0, 0, 0, .75);
}

void scene_renderer::resize(size sz) { glViewport(0, 0, sz.width, sz.height); }

void scene_renderer::draw(clock::time_point ts [[maybe_unused]]) {
  FrameMark;
  ZoneScopedN("render frame");
  // fetch phases from controller

  // calculate uniforms

  // do render
  {
    ZoneScopedN("draw calls");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  {
    ZoneScopedN("glFlush");
    glFlush();
  }
}
