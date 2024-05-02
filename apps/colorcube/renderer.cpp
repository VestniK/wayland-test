#include "renderer.hpp"

#include <Tracy.hpp>

#include <scene/controller.hpp>
#include <scene/landscape.hpp>
#include <scene/mesh_data.hpp>

#include <libs/gles2/textures.hpp>

#include <apps/colorcube/shaders.hpp>

using namespace std::literals;

namespace {

// clang-format off
const vertex cube_vertices[] = {
    {{-1., 1., -1.}, {0., 0., -1.}},
    {{1., 1., -1.}, {0., 0., -1.}},
    {{-1., -1., -1.}, {0., 0., -1.}},
    {{1., -1., -1.}, {0., 0., -1.}},
    
    {{1., 1., -1.}, {1., 0., 0.}},
    {{1., 1., 1.}, {1., 0., 0.}},
    {{1., -1., -1.}, {1., 0., 0.}},
    {{1., -1., 1.}, {1., 0., 0.}},
    
    {{-1., 1., -1.}, {-1., 0., 0.}},
    {{-1., 1., 1.}, {-1., 0., 0.}},
    {{-1., -1., -1.}, {-1., 0., 0.}},
    {{-1., -1., 1.}, {-1., 0., 0.}},
    
    {{-1., 1., -1.}, {0., 1., 0.}},
    {{1., 1., -1.}, {0., 1., 0.}},
    {{1., 1., 1.}, {0., 1., 0.}},
    {{-1., 1., 1.}, {0., 1., 0.}},
    
    {{-1., -1., -1.}, {0., -1., 0.}},
    {{1., -1., -1.}, {0., -1., 0.}},
    {{1., -1., 1.}, {0., -1., 0.}},
    {{-1., -1., 1.}, {0., -1., 0.}},
    
    {{-1., 1., 1.}, {0., 0., 1.}},
    {{1., 1., 1.}, {0., 0., 1.}},
    {{-1., -1., 1.}, {0., 0., 1.}},
    {{1., -1., 1.}, {0., 0., 1.}}
};
const GLuint cube_idxs[] = {
    0, 1, 2, 1, 3, 2,
    
    4, 5, 6, 6, 5, 7,
    
    8, 9, 10, 10, 9, 11,
    
    12, 13, 14, 14, 12, 15,
    
    16, 17, 18, 18, 16, 19,
    
    20, 21, 22, 21, 23, 22
};
// clang-format on

glm::mat4 animate_cube_pos(
    glm::vec2 planar_pos, frames_clock::time_point ts) noexcept {
  constexpr auto period = 5s;
  constexpr auto flyght_period = 7s;

  const GLfloat spin_phase = (ts.time_since_epoch() % period).count() /
                             GLfloat(frames_clock::duration{period}.count());
  const GLfloat flyght_phase =
      (ts.time_since_epoch() % flyght_period).count() /
      GLfloat(frames_clock::duration{flyght_period}.count());

  const GLfloat angle = 2 * M_PI * spin_phase;

  return glm::translate(
             glm::mat4{1.}, glm::vec3{planar_pos,
                                3. + 2. * std::cos(2 * M_PI * flyght_phase)}) *
         glm::rotate(glm::mat4{1.}, angle, {.5, .3, .1}) *
         glm::scale(glm::mat4{1.}, {.5, .5, .5});
}

} // namespace

scene_renderer::scene_renderer(const scene::controller& contr)
    : controller_{contr}, cube_{cube_vertices, cube_idxs} {
  landscape land{centimeters{5}, 120, 80};
  landscape_ = mesh{land.verticies(), land.indexes()};

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0, 0, 0, .75);
}

void scene_renderer::resize(size sz) {
  glViewport(0, 0, sz.width, sz.height);

  constexpr auto camera_pos_ = glm::vec3{7., 12., 18.};
  constexpr auto camera_look_at = glm::vec3{3., 2., 0.};
  constexpr auto camera_up_direction_ = glm::vec3{0., 0., 1.};
  camera_ =
      glm::perspectiveFov<float>(M_PI / 6., sz.width, sz.height, 10.f, 35.f) *
      glm::lookAt(camera_pos_, camera_look_at, camera_up_direction_);
}

void scene_renderer::draw(clock::time_point ts) {
  FrameMark;
  ZoneScopedN("render frame");
  // fetch phases from controller
  if (const auto cube_color = controller_.get_cube_color_update())
    cube_color_.reset(ts, cube_color.value());
  if (const auto landscape_color = controller_.get_landscape_color_update())
    landscape_color_.reset(ts, landscape_color.value());
  const auto cube_vel = controller_.current_cube_vel();

  // calculate uniforms
  const auto landscape_color = landscape_color_.animate(ts);
  const auto cube_color = cube_color_.animate(ts);
  const glm::mat4 model =
      animate_cube_pos(cube_pos_integrator_(cube_vel, ts), ts);

  // do render
  {
    ZoneScopedN("draw calls");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    pipeline_.start_rendering(camera_);
    pipeline_.draw(glm::mat4{1.}, landscape_color, landscape_);
    pipeline_.draw(model, cube_color, cube_);
  }

  {
    ZoneScopedN("glFlush");
    glFlush();
  }
}
