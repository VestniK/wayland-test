#include <stdexcept>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <wayland/gles2/renderer.hpp>

using namespace std::literals;

struct vertex {
  glm::vec3 position;
  glm::vec3 normal;
};

renderer::renderer()
    : vertex_shader_{compile(shader_type::vertex, R"(
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
    )")},
      fragment_shader_{compile(shader_type::fragment, R"(
      #version 100
      precision mediump float;

      uniform vec3 light_pos;
      uniform mat4 model;
      uniform mat3 norm_rotation;

      varying vec3 frag_normal;
      varying vec3 frag_pos;

      void main() {
        vec3 light_vector = light_pos - vec3(model*vec4(frag_pos, 1.0));
        float brightnes = dot(normalize(norm_rotation*frag_normal), light_vector)/length(light_vector);
        brightnes = clamp(brightnes, 0.0, 1.0);
        gl_FragColor = vec4(brightnes*vec3(0.0, 0.0, 1.0), 1.0);
      }
    )")},
      program_{link(vertex_shader_, fragment_shader_)}, ibo_{gen_buffer()},
      vbo_{gen_buffer()} {
  // clang-format off
  static const vertex vertices[] = {
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
  // clang-format on
  glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // clang-format off
  static const GLuint idxs[] = {
    0, 1, 2, 1, 3, 2,

    4, 5, 6, 6, 5, 7,

    8, 9, 10, 10, 9, 11,

    12, 13, 14, 14, 12, 15,

    16, 17, 18, 18, 16, 19,

    20, 21, 22, 21, 23, 22
  };
  // clang-format on
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

  glUseProgram(program_.get());

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0, 0, 0, .9);
}

void renderer::resize(size sz) {
  sz_ = sz;
  const float len = std::min(sz.width, sz.height);
  glViewport(0, 0, sz.width, sz.height);
  const glm::mat4 camera = glm::frustum(-sz.width / len, sz.width / len,
                               -sz.height / len, sz.height / len, 3.f, 8.f) *
                           glm::lookAt(glm::vec3{.0, .0, .0},
                               glm::vec3{.0, .0, 2.}, glm::vec3{.0, 1., .0});

  const GLint camera_uniform = glGetUniformLocation(program_.get(), "camera");
  glUniformMatrix4fv(camera_uniform, 1, GL_FALSE, glm::value_ptr(camera));
}

void renderer::draw(clock::time_point ts) {
  constexpr auto period = 3s;
  constexpr auto spot_period = 7s;

  const GLfloat phase = (ts.time_since_epoch() % period).count() /
                        GLfloat(clock::duration{period}.count());
  const GLfloat spot_phase = (ts.time_since_epoch() % spot_period).count() /
                             GLfloat(clock::duration{spot_period}.count());

  const GLfloat spot_angle = 2 * M_PI * spot_phase;
  const GLfloat angle = 2 * M_PI * phase;
  glm::vec3 light_pos = {
      3. * std::cos(spot_angle), 6. * std::sin(2.5 * spot_angle), 0.0};
  glm::mat4 model = glm::translate(glm::vec3{0, 0, 6}) *
                    glm::rotate(glm::mat4{1.}, angle, {.5, .3, .1});
  glm::mat3 norm_rotation = glm::transpose(glm::inverse(glm::mat3(model)));

  const GLint norm_rotation_uniform =
      glGetUniformLocation(program_.get(), "norm_rotation");
  const GLint model_uniform = glGetUniformLocation(program_.get(), "model");
  const GLint light_pos_uniform =
      glGetUniformLocation(program_.get(), "light_pos");
  const GLint position_location =
      glGetAttribLocation(program_.get(), "position");
  const GLint normal_location = glGetAttribLocation(program_.get(), "normal");

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
  glUniformMatrix3fv(
      norm_rotation_uniform, 1, GL_FALSE, glm::value_ptr(norm_rotation));
  glUniform3fv(light_pos_uniform, 1, glm::value_ptr(light_pos));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
  glVertexAttribPointer(
      position_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), nullptr);
  glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
      reinterpret_cast<const GLvoid*>(sizeof(glm::vec3)));
  glEnableVertexAttribArray(position_location);
  glEnableVertexAttribArray(normal_location);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
  glFlush();
}
