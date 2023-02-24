#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <wayland/gles2/landscape.hpp>
#include <wayland/gles2/mesh_data.hpp>
#include <wayland/gles2/renderer.hpp>

#include <wayland/gles2/shaders/shaders.hpp>

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

void apply_model_world_transformation(
    glm::mat4 transformation, uniform_location<glm::mat4> model_mat_uniform,
    uniform_location<glm::mat3> normal_mat_uniform) {
  model_mat_uniform.set_value(transformation);
  normal_mat_uniform.set_value(
      glm::transpose(glm::inverse(glm::mat3(transformation))));
}

} // namespace

mesh::mesh(std::span<const vertex> verticies, std::span<const GLuint> indexes)
    : ibo_{gen_buffer()}, vbo_{gen_buffer()},
      triangles_count_{static_cast<unsigned>(indexes.size())} {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
  glBufferData(GL_ARRAY_BUFFER, verticies.size_bytes(), verticies.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size_bytes(), indexes.data(),
               GL_STATIC_DRAW);
}

void mesh::draw(shader_pipeline::attributes attrs) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
  attrs.position.set_pointer(&vertex::position);
  attrs.normal.set_pointer(&vertex::normal);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glDrawElements(GL_TRIANGLES, triangles_count_, GL_UNSIGNED_INT, nullptr);
}

shader_pipeline::shader_pipeline()
    : shader_prog_{shaders::main_vert, shaders::main_frag},
      attributes_{.position = shader_prog_.get_attrib<glm::vec3>("position"),
                  .normal = shader_prog_.get_attrib<glm::vec3>("normal")} {
  model_world_uniform_ = shader_prog_.get_uniform<glm::mat4>("model");
  norm_world_uniform_ = shader_prog_.get_uniform<glm::mat3>("norm_rotation");

  camera_uniform_ = shader_prog_.get_uniform<glm::mat4>("camera");
  color_uniform_ = shader_prog_.get_uniform<glm::vec3>("color");

  shader_prog_.use();
  shader_prog_.get_uniform<float>("light.intense").set_value(0.8);
  shader_prog_.get_uniform<float>("light.ambient").set_value(0.4);
  shader_prog_.get_uniform<float>("light.attenuation").set_value(0.01);
  shader_prog_.get_uniform<glm::vec3>("light.pos").set_value({2., 5., 15.});
};

void shader_pipeline::start_rendering(glm::mat4 camera) {
  shader_prog_.use();
  camera_uniform_.set_value(camera);
}

void shader_pipeline::draw(glm::mat4 model, glm::vec3 color, mesh &mesh) {
  apply_model_world_transformation(model, model_world_uniform_,
                                   norm_world_uniform_);
  color_uniform_.set_value(color);
  mesh.draw(attributes_);
}

scene_renderer::scene_renderer() : cube_{cube_vertices, cube_idxs} {

  landscape land{centimeters{5}, 120, 80};
  landscape_ = mesh{land.verticies(), land.indexes()};

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0, 0, 0, .75);
}

void scene_renderer::resize(size sz) {
  glViewport(0, 0, sz.width, sz.height);
  projection_ =
      glm::perspectiveFov<float>(M_PI / 6., sz.width, sz.height, 10.f, 35.f);
}

void scene_renderer::draw(clock::time_point ts) {
  constexpr auto period = 3s;
  constexpr auto spot_period = 27s;

  const GLfloat spin_phase = (ts.time_since_epoch() % period).count() /
                             GLfloat(clock::duration{period}.count());
  const GLfloat flyght_phase = (ts.time_since_epoch() % spot_period).count() /
                               GLfloat(clock::duration{spot_period}.count());

  const GLfloat spot_angle = 2 * M_PI * flyght_phase;
  const GLfloat angle = 2 * M_PI * spin_phase;

  const glm::mat4 camera =
      glm::lookAt(glm::vec3{7, 10, 18},
                  glm::vec3{3 + 2 * std::cos(6 * M_PI * flyght_phase),
                            1 + 2 * std::sin(4 * M_PI * flyght_phase), 0},
                  glm::vec3{.0, .0, 1.});

  const glm::mat4 model =
      glm::translate(glm::mat4{1.},
                     glm::vec3{4 + 2 * std::cos(3 * spot_angle),
                               2.5 - 0.6 * std::sin(5 * spot_angle),
                               1.5 + std::cos(spot_angle)}) *
      glm::rotate(glm::mat4{1.}, angle, {.5, .3, .1}) *
      glm::scale(glm::mat4{1.}, {.5, .5, .5});

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  pipeline_.start_rendering(projection_ * camera);
  pipeline_.draw(model, {.9, 0.7, 0.7}, cube_);
  pipeline_.draw(glm::mat4{1.}, {1., 1., 0.4}, landscape_);

  glFlush();
}
