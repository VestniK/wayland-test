#include <stdexcept>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <wayland/gles2/renderer.hpp>

using namespace std::literals;

renderer::renderer()
    : vertex_shader_{compile(shader_type::vertex, R"(
      uniform mat4 camera;
      uniform mat4 rotation;
      attribute vec2 position;
      void main() {
        gl_Position = camera * rotation * vec4(position.xy, 1., 1.);
      }
    )")},
      fragment_shader_{compile(shader_type::fragment, R"(
      precision mediump float;
      uniform vec4 hotspot;
      void main() {
        float norm_dist = length(gl_FragCoord - hotspot)/90.0;
        float factor = exp(-norm_dist*norm_dist);
        vec4 color = vec4(0.0, 0.0, 1.0, 1.0);
        gl_FragColor = vec4(color.r*factor, color.g*factor, color.b*factor, 1);
      }
    )")},
      program_{link(vertex_shader_, fragment_shader_)}, ibo_{gen_buffer()},
      vbo_{gen_buffer()} {
  static const glm::vec2 vertices[] = {
      {-1., 1.}, {1., 1.}, {-1., -1.}, {1., -1.}};
  glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  static const GLuint idxs[] = {0, 1, 2, 1, 3, 2};
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

  glUseProgram(program_.get());
}

void renderer::resize(size sz) {
  glClearColor(0, 0, 0, .9);
  sz_ = sz;
  const float len = std::min(sz.width, sz.height);
  glViewport(0, 0, sz.width, sz.height);
  const glm::mat4 camera = glm::frustum(-sz.width / len, sz.width / len,
                               -sz.height / len, sz.height / len, .5f, 2.f) *
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
  glm::vec4 hotspot = {
      sz_.width / 2 + 7 * sz_.width / 24 * std::cos(spot_angle),
      5 * sz_.height / 12 + sz_.height / 24 * std::cos(2 * spot_angle), 0, 0};
  glm::mat4 rotation = glm::rotate(glm::mat4{1.}, angle, {0, 0, 1});

  const GLint rotation_uniform =
      glGetUniformLocation(program_.get(), "rotation");
  const GLint hotspot_uniform = glGetUniformLocation(program_.get(), "hotspot");
  const GLint position_location =
      glGetAttribLocation(program_.get(), "position");

  glClear(GL_COLOR_BUFFER_BIT);

  glUniformMatrix4fv(rotation_uniform, 1, GL_FALSE, glm::value_ptr(rotation));
  glUniform4fv(hotspot_uniform, 1, glm::value_ptr(hotspot));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_.get());
  glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(position_location);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_.get());
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
  glFlush();
}
