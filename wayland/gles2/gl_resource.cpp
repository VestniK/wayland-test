#include <wayland/gles2/gl_resource.hpp>

shader compile(shader_type type, gsl::czstring<> src) {
  return compile(type, gsl::span<gsl::czstring<>>{&src, 1});
}

shader compile(shader_type type, gsl::span<gsl::czstring<>> srcs) {
  shader res{glCreateShader(static_cast<GLenum>(type))};
  if (!res)
    throw std::runtime_error{"Failed to create shader"};
  glShaderSource(res.get(), srcs.size(), srcs.data(), nullptr);
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

shader_program link(const shader& vertex_shader, const shader& fragment_shader) {
  shader_program res{glCreateProgram()};
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