#include <stdexcept>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <wayland/renderer.hpp>

using namespace std::literals;

shader compile(shader_type type, gsl::czstring<> src) {
  shader res{glCreateShader(static_cast<GLenum>(type))};
  if (!res)
    throw std::runtime_error{"Failed to create shader"};
  glShaderSource(res.get(), 1, &src, nullptr);
  glCompileShader(res.get());
  GLint compiled;
  glGetShaderiv(res.get(), GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint msg_size;
    glGetShaderiv(res.get(), GL_INFO_LOG_LENGTH, &msg_size);
    std::string msg;
    if (msg_size > 1) {
      msg.resize(msg_size);
      glGetShaderInfoLog(res.get(), msg_size, nullptr, msg.data());
    } else
      msg = "unknown compiation error";
    throw std::runtime_error{"Failed to compile shader: " + msg};
  }
  return res;
}

program link(const shader& vertex_shader, const shader& fragment_shader) {
  program res{glCreateProgram()};
  if (!res)
    throw std::runtime_error{"Failed to create GLSL program"};
  glAttachShader(res.get(), vertex_shader.get());
  glAttachShader(res.get(), fragment_shader.get());
  glBindAttribLocation(res.get(), 0, "vPosition");
  glLinkProgram(res.get());

  GLint linked;
  glGetProgramiv(res.get(), GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint msg_size = 0;
    glGetProgramiv(res.get(), GL_INFO_LOG_LENGTH, &msg_size);
    std::string msg;
    if (msg_size > 1) {
      msg.resize(msg_size);
      glGetProgramInfoLog(res.get(), msg_size, nullptr, msg.data());
    } else
      msg = "unknown link error";
    throw std::runtime_error{"Faild to link GLSL program: " + msg};
  }
  return res;
}

renderer::renderer()
    : vertex_shader_{compile(shader_type::vertex, R"(
      uniform mat4 rotation;
      attribute vec4 position;
      void main() {
        gl_Position = rotation * position;
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
      program_{link(vertex_shader_, fragment_shader_)} {
  static const GLuint idxs[] = {0, 1, 2, 1, 3, 2};
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxs_.native_handle());
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);
}

void renderer::resize(size sz) {
  glClearColor(0, 0, 0, .9);
  len_ = std::min(sz.width, sz.height);
  glViewport(0, 0, len_, len_);
}

void renderer::draw(clock::time_point ts) {
  struct vertex {
    GLfloat x = 0;
    GLfloat y = 0;
  };
  static const vertex vertices[] = {{-.5, .5}, {.5, .5}, {-.5, -.5}, {.5, -.5}};

  constexpr auto period = 3s;
  constexpr auto spot_period = 7s;

  const GLfloat phase = (ts.time_since_epoch() % period).count() /
                        GLfloat(clock::duration{period}.count());
  const GLfloat spot_phase = (ts.time_since_epoch() % spot_period).count() /
                             GLfloat(clock::duration{spot_period}.count());

  const GLfloat spot_angle = 2 * M_PI * spot_phase;
  const GLfloat angle = 2 * M_PI * phase;
  glm::vec4 hotspot = {len_ / 2 + 7 * len_ / 24 * std::cos(spot_angle),
      5 * len_ / 12 + len_ / 24 * std::cos(2 * spot_angle), 0, 0};
  glm::mat4 rotation = glm::rotate(glm::mat4{1.}, angle, {0, 0, 1});

  GLint rotation_uniform = glGetUniformLocation(program_.get(), "rotation");
  GLint hotspot_uniform = glGetUniformLocation(program_.get(), "hotspot");
  GLint position_location = glGetAttribLocation(program_.get(), "position");

  glClear(GL_COLOR_BUFFER_BIT);

  glUniformMatrix4fv(rotation_uniform, 1, GL_FALSE, glm::value_ptr(rotation));
  glUniform4fv(hotspot_uniform, 1, glm::value_ptr(hotspot));

  glUseProgram(program_.get());
  glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glEnableVertexAttribArray(position_location);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxs_.native_handle());
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
  glFlush();
}
