#include <vector>

#include <Tracy/tracy/Tracy.hpp>

#include <scene/mesh_data.hpp>

#include <gles2/renderer.hpp>
#include <gles2/shaders.hpp>
#include <gles2/textures.hpp>

using namespace std::literals;

scene_renderer::scene_renderer(const scene::controller& contr)
    : controller_{contr} {
  constexpr int x_segments = 170;
  constexpr int y_segments = 210;

  std::vector<vertex> vertices;
  std::vector<GLuint> indices;
  std::vector<std::byte> morph_texture_data;
  vertices.reserve(x_segments * y_segments);
  indices.reserve(3 * (x_segments - 1) * (y_segments - 1));
  morph_texture_data.reserve(4 * x_segments * y_segments);

  auto pos2idx = [](int x, int y) -> GLuint { return y * x_segments + x; };

  for (int y = 0; y < y_segments; ++y) {
    for (int x = 0; x < x_segments; ++x) {
      vertices.push_back({.position = {0.1 * x, 0.1 * y, 0.},
          .normal = {0., 0., 1.},
          .uv = {0.1 * x, 0.1 * y},
          .idxs{static_cast<float>(x) / x_segments,
              static_cast<float>(y) / y_segments}});

      morph_texture_data.push_back(std::byte{0});
      morph_texture_data.push_back(std::byte{0});
      morph_texture_data.push_back(std::byte{static_cast<std::uint8_t>(
          0xff * ((1. + std::cos(M_PI * (x + y) / 60.)) / 2.))});
      morph_texture_data.push_back(std::byte{0});

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
  gl::samplers[0]
      .bind(morph_[0])
      .image_2d(gl::texel_format::rgba, size{x_segments, y_segments},
          morph_texture_data)
      .tex_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
      .tex_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
      .tex_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT)
      .tex_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
  pipeline_.bind_morph(gl::samplers[0]);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0, 0, 0, .75);
}

void scene_renderer::resize(size sz) {
  glViewport(0, 0, sz.width, sz.height);

  constexpr auto camera_pos_ = glm::vec3{0., 0., 50.};
  constexpr auto camera_up_direction_ = glm::vec3{0., 1., 0.};
  constexpr auto camera_look_at = glm::vec3{10., 10., 0.};
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
