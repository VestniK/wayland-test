#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <gles2/landscape.hpp>
#include <gles2/mesh_data.hpp>
#include <gles2/renderer.hpp>
#include <gles2/shaders/shaders.hpp>

using namespace std::literals;

namespace {

// clang-format off
const vertex cube_vertices[] = {
  {{-1., 1., -1.}, {0., 0., -1.}, {0., .5}},
  {{1., 1., -1.}, {0., 0., -1.}, {.5, .5}},
  {{-1., -1., -1.}, {0., 0., -1.}, {0., 0.}},
  {{1., -1., -1.}, {0., 0., -1.}, {.5, 0.}},

  {{1., 1., -1.}, {1., 0., 0.}, {.75, .25}},
  {{1., 1., 1.}, {1., 0., 0.}, {.75, .75}},
  {{1., -1., -1.}, {1., 0., 0.}, {.25, .25}},
  {{1., -1., 1.}, {1., 0., 0.}, {.25, .75}},

  {{-1., 1., -1.}, {-1., 0., 0.}, {1., .5}},
  {{-1., 1., 1.}, {-1., 0., 0.}, {1., 1.}},
  {{-1., -1., -1.}, {-1., 0., 0.}, {.5, .5}},
  {{-1., -1., 1.}, {-1., 0., 0.}, {.5, 1.}},

  {{-1., 1., -1.}, {0., 1., 0.}, {.5, .5}},
  {{1., 1., -1.}, {0., 1., 0.}, {1., .5}},
  {{1., 1., 1.}, {0., 1., 0.}, {.1, 1.}},
  {{-1., 1., 1.}, {0., 1., 0.}, {.5, 1.}},

  {{-1., -1., -1.}, {0., -1., 0.}, {0., .5}},
  {{1., -1., -1.}, {0., -1., 0.}, {.5, .5}},
  {{1., -1., 1.}, {0., -1., 0.}, {.5, 1.}},
  {{-1., -1., 1.}, {0., -1., 0.}, {0., 1.}},

  {{-1., 1., 1.}, {0., 0., 1.}, {.5, .5}},
  {{1., 1., 1.}, {0., 0., 1.}, {1., .5}},
  {{-1., -1., 1.}, {0., 0., 1.}, {.5, 0.}},
  {{1., -1., 1.}, {0., 0., 1.}, {1., 0.}}
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

void apply_model_world_transformation(glm::mat4 transformation,
    uniform_location<glm::mat4> model_mat_uniform,
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
  attrs.uv.set_pointer(&vertex::uv);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glDrawElements(GL_TRIANGLES, triangles_count_, GL_UNSIGNED_INT, nullptr);
}

shader_pipeline::shader_pipeline()
    : shader_prog_{shaders::main_vert, shaders::main_frag},
      attributes_{.position = shader_prog_.get_attrib<glm::vec3>("position"),
          .normal = shader_prog_.get_attrib<glm::vec3>("normal"),
          .uv = shader_prog_.get_attrib<glm::vec2>("uv")} {
  model_world_uniform_ = shader_prog_.get_uniform<glm::mat4>("model");
  norm_world_uniform_ = shader_prog_.get_uniform<glm::mat3>("norm_rotation");

  camera_uniform_ = shader_prog_.get_uniform<glm::mat4>("camera");
  texture_index_uniform_ = shader_prog_.get_uniform<GLint>("texture_data");
  tex_offset_uniform_ = shader_prog_.get_uniform<glm::vec2>("tex_offset");

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

void shader_pipeline::draw(
    glm::mat4 model, int tex_idx, mesh& mesh, glm::vec2 tex_offset) {
  apply_model_world_transformation(
      model, model_world_uniform_, norm_world_uniform_);
  tex_offset_uniform_.set_value(tex_offset);
  texture_index_uniform_.set_value(tex_idx);
  mesh.draw(attributes_);
}

scene_renderer::scene_renderer(
    img::image cube_tex, img::image land_tex, const controller& contr)
    : cube_{cube_vertices, cube_idxs}, cube_tex_{gen_texture()},
      land_tex_{gen_texture()}, controller_{contr} {
  landscape land{centimeters{5}, 120, 80};
  landscape_ = mesh{land.verticies(), land.indexes()};

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0, 0, 0, .75);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, cube_tex_.get());
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cube_tex.size().width,
      cube_tex.size().height, 0, GL_RGB, GL_UNSIGNED_BYTE,
      cube_tex.data().data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, land_tex_.get());
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, land_tex.size().width,
      land_tex.size().height, 0, GL_RGB, GL_UNSIGNED_BYTE,
      land_tex.data().data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

  const glm::mat4 camera = glm::lookAt(glm::vec3{7, 10, 18},
      glm::vec3{3 + 2 * std::cos(6 * M_PI * flyght_phase),
          1 + 2 * std::sin(4 * M_PI * flyght_phase), 0},
      glm::vec3{.0, .0, 1.});

  const auto cube_vel_raw = controller_.current_cube_vel();
  const auto cube_vel = glm::vec2{-cube_vel_raw.x, cube_vel_raw.y} / 8000.f;
  cube_planar_movement_.cur_pos +=
      std::chrono::duration_cast<float_time::seconds>(
          ts - std::exchange(cube_planar_movement_.last_ts, ts))
          .count() *
      cube_vel;
  cube_planar_movement_.cur_pos.x =
      std::clamp(cube_planar_movement_.cur_pos.x, -4.f, 4.f);
  cube_planar_movement_.cur_pos.y =
      std::clamp(cube_planar_movement_.cur_pos.y, -4.f, 4.f);

  const glm::mat4 model = glm::translate(glm::mat4{1.},
                              glm::vec3{cube_planar_movement_.cur_pos.x + 3.5,
                                  cube_planar_movement_.cur_pos.y + 3.,
                                  2. + std::cos(spot_angle)}) *
                          glm::rotate(glm::mat4{1.}, angle, {.5, .3, .1}) *
                          glm::scale(glm::mat4{1.}, {.5, .5, .5});

  if (const auto tex_offset = controller_.cube_tex_offset_update())
    cube_tex_offset_.reset(ts, tex_offset.value());

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  pipeline_.start_rendering(projection_ * camera);
  glActiveTexture(GL_TEXTURE0);
  pipeline_.draw(model, 0, cube_, cube_tex_offset_.animate(ts));
  glActiveTexture(GL_TEXTURE1);
  pipeline_.draw(glm::mat4{1.}, 1, landscape_);

  glFlush();
}
