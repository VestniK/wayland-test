#include <vector>

#include <Tracy/tracy/Tracy.hpp>

#include <scene/mesh_data.hpp>

#include <gles2/renderer.hpp>
#include <gles2/shaders.hpp>
#include <gles2/textures.hpp>

using namespace std::literals;

scene_renderer::scene_renderer(const scene::controller& contr)
    : controller_{contr} {
  std::vector<vertex> vertices;
  std::vector<GLuint> indices;

  constexpr int x_segments = 100;
  constexpr int y_segments = 100;
  auto pos2idx = [](int x, int y) -> GLuint { return y * x_segments + x; };

  for (int y = 0; y < y_segments; ++y) {
    for (int x = 0; x < x_segments; ++x) {
      vertices.push_back({.position = {0.1 * x, 0.1 * y, 0.},
          .normal = {0., 0., 1.},
          .uv = {0.1 * x, 0.1 * y}});
      if (y > 0 && x < x_segments - 1) {
        indices.push_back(pos2idx(x, y - 1));
        indices.push_back(pos2idx(x + 1, y - 1));
        indices.push_back(pos2idx(x, y));

        indices.push_back(pos2idx(x + 1, y - 1));
        indices.push_back(pos2idx(x + 1, y));
        indices.push_back(pos2idx(x, y));
      }
    }
  }
  paper_ = mesh{vertices, indices};

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0, 0, 0, .75);
}

void scene_renderer::resize(size sz) {
  glViewport(0, 0, sz.width, sz.height);

  constexpr auto camera_pos_ = glm::vec3{0., 0., 50.};
  constexpr auto camera_up_direction_ = glm::vec3{0., 1., 0.};
  constexpr auto camera_look_at = glm::vec3{0., 0, 0.};
  camera_ =
      glm::perspectiveFov<float>(M_PI / 6., sz.width, sz.height, 30.f, 60.f) *
      glm::lookAt(camera_pos_, camera_look_at, camera_up_direction_);
}

void scene_renderer::draw(clock::time_point ts [[maybe_unused]]) {
  FrameMark;
  ZoneScopedN("render frame");
  // fetch phases from controller

  // calculate uniforms

  // do render
  {
    ZoneScopedN("draw calls");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    pipeline_.start_rendering(camera_);
    pipeline_.draw(glm::mat4{1.}, paper_);
  }

  {
    ZoneScopedN("glFlush");
    glFlush();
  }
}
