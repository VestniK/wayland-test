#include <vector>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <wayland/gles2/landscape.hpp>
#include <wayland/gles2/mesh_data.hpp>
#include <wayland/gles2/renderer.hpp>

using namespace std::literals;

namespace {

gsl::czstring<> vertex_shader[] = {
    R"(
    #version 100
    precision mediump float;

    uniform mat4 camera;
    uniform mat4 model;

    attribute vec3 position;
    attribute vec3 normal;

    varying vec3 frag_normal;
    varying vec3 frag_pos;

    void main() {
      frag_normal = normal;
      frag_pos = position;
      gl_Position = camera * model * vec4(position.xyz, 1.);
    }
)"};

gsl::czstring<> fragment_shaders[] = {
    R"(
      #version 100
      precision mediump float;)",
    R"(
      struct light_source {
        vec3 pos;
        float intense;
        float ambient;
        float attenuation;
      };

      vec3 phong_reflect(light_source light, vec3 surf_color, vec3 world_pos, vec3 world_normal) {
        vec3 light_direction = normalize(light.pos - world_pos);
        float brightnes = light.intense*dot(world_normal, light_direction);
        brightnes = clamp(brightnes, 0.0, 1.0);
        vec3 diffuse = brightnes*surf_color;
        vec3 ambient = light.intense*light.ambient*surf_color;
        float attenuation = 1.0/(1.0 + light.attenuation*pow(length(light.pos - world_pos), 2.0));
        return ambient + attenuation*diffuse;
      }
    )",
    R"(
      uniform light_source light;
      uniform mat4 model;
      uniform mat3 norm_rotation;

      varying vec3 frag_normal;
      varying vec3 frag_pos;

      void main() {
        vec3 color = vec3(0.0, 0.7, 0.7);

        gl_FragColor = vec4(
          phong_reflect(light, color, vec3(model*vec4(frag_pos, 1.0)), normalize(norm_rotation*frag_normal)),
          1.0
        );
      }
    )"};

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

void apply_model_world_transformation(glm::mat4 transformation,
    uniform_location<glm::mat4> model_mat_uniform,
    uniform_location<glm::mat3> normal_mat_uniform) {
  model_mat_uniform.set_value(transformation);
  normal_mat_uniform.set_value(
      glm::transpose(glm::inverse(glm::mat3(transformation))));
}

} // namespace

shader_pipeline::shader_pipeline(
    gsl::span<gsl::czstring<>> vertex_shader_sources,
    gsl::span<gsl::czstring<>> fragment_shader_sources)
    : program_{link(compile(shader_type::vertex, vertex_shader_sources),
          compile(shader_type::fragment, fragment_shader_sources))} {}

void shader_pipeline::use() { glUseProgram(program_.get()); }

mesh::mesh(gsl::span<const vertex> verticies, gsl::span<const GLuint> indexes)
    : ibo_{gen_buffer()}, vbo_{gen_buffer()}, triangles_count_{
                                                  static_cast<unsigned>(
                                                      indexes.size())} {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
  glBufferData(GL_ARRAY_BUFFER, verticies.size_bytes(), verticies.data(),
      GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size_bytes(), indexes.data(),
      GL_STATIC_DRAW);
}

void mesh::draw(shader_pipeline& pipeline, gsl::czstring<> pos_name,
    gsl::czstring<> normal_name) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
  pipeline.set_attrib_pointer(pos_name, &vertex::position);
  pipeline.set_attrib_pointer(normal_name, &vertex::normal);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glDrawElements(GL_TRIANGLES, triangles_count_, GL_UNSIGNED_INT, nullptr);
}

renderer::renderer()
    : pipeline_{vertex_shader, fragment_shaders}, cube_{cube_vertices,
                                                      cube_idxs} {
  pipeline_.use();
  pipeline_.get_uniform<float>("light.intense").set_value(0.8);
  pipeline_.get_uniform<float>("light.ambient").set_value(0.3);
  pipeline_.get_uniform<float>("light.attenuation").set_value(0.01);
  pipeline_.get_uniform<glm::vec3>("light.pos").set_value({2., 5., 15.});
  model_world_uniform_ = pipeline_.get_uniform<glm::mat4>("model");
  norm_world_uniform_ = pipeline_.get_uniform<glm::mat3>("norm_rotation");

  camera_uniform_ = pipeline_.get_uniform<glm::mat4>("camera");

  const mesh_data land_data = generate_flat_landscape(.5, 5, 8);
  landscape_ = mesh{land_data.verticies, land_data.indexes};

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0, 0, 0, .75);
}

void renderer::resize(size sz) {
  sz_ = sz;
  glViewport(0, 0, sz.width, sz.height);
  const glm::mat4 camera =
      glm::perspectiveFov<float>(M_PI / 6., sz.width, sz.height, 10.f, 35.f) *
      glm::lookAt(glm::vec3{.0, .0, 20.}, glm::vec3{4., 2.5, .0},
          glm::vec3{.0, .0, 1.});

  camera_uniform_.set_value(camera);
}

void renderer::draw(clock::time_point ts) {
  constexpr auto period = 3s;
  constexpr auto spot_period = 27s;

  const GLfloat spin_phase = (ts.time_since_epoch() % period).count() /
                             GLfloat(clock::duration{period}.count());
  const GLfloat flyght_phase = (ts.time_since_epoch() % spot_period).count() /
                               GLfloat(clock::duration{spot_period}.count());

  const GLfloat spot_angle = 2 * M_PI * flyght_phase;
  const GLfloat angle = 2 * M_PI * spin_phase;

  glm::mat4 model =
      glm::translate(glm::vec3{4 + 2 * std::cos(3 * spot_angle),
          2.5 - 0.6 * std::sin(5 * spot_angle), 1.5 + std::cos(spot_angle)}) *
      glm::rotate(glm::mat4{1.}, angle, {.5, .3, .1}) *
      glm::scale(glm::mat4{1.}, {.5, .5, .5});

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  apply_model_world_transformation(
      model, model_world_uniform_, norm_world_uniform_);
  cube_.draw(pipeline_, "position", "normal");
  apply_model_world_transformation(glm::translate(glm::vec3{1.5, 0., 0.}),
      model_world_uniform_, norm_world_uniform_);
  landscape_.draw(pipeline_, "position", "normal");
  glFlush();
}
