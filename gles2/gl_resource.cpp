#include <stdexcept>

#include <gles2/gl_resource.hpp>

namespace gl {

shader compile(shader_type type, const char* src) {
  return compile(type, std::span<const char*>{&src, 1});
}

shader compile(shader_type type, std::span<const char*> srcs) {
  shader res{shader_handle{glCreateShader(static_cast<GLenum>(type))}};
  if (!res)
    throw std::runtime_error{"Failed to create shader"};
  glShaderSource(res.native_handle(), srcs.size(), srcs.data(), nullptr);
  glCompileShader(res.native_handle());
  GLint compiled;
  glGetShaderiv(res.native_handle(), GL_COMPILE_STATUS, &compiled);
  if (compiled == 0) {
    GLint msg_size;
    glGetShaderiv(res.native_handle(), GL_INFO_LOG_LENGTH, &msg_size);
    std::string msg;
    if (msg_size > 1) {
      msg.resize(msg_size);
      glGetShaderInfoLog(res.native_handle(), msg_size, nullptr, msg.data());
    } else
      msg = "unknown compiation error";
    throw std::runtime_error{"Failed to compile shader: " + msg};
  }
  return res;
}

resource<shader_program_handle> link(
    shader_handle vertex, shader_handle fragment) {
  resource<shader_program_handle> res{shader_program_handle{glCreateProgram()}};
  if (!res)
    throw std::runtime_error{"Failed to create GLSL program"};
  glAttachShader(res.native_handle(), vertex.native_handle());
  glAttachShader(res.native_handle(), fragment.native_handle());
  glBindAttribLocation(res.native_handle(), 0, "vPosition");
  glLinkProgram(res.native_handle());

  GLint linked;
  glGetProgramiv(res.native_handle(), GL_LINK_STATUS, &linked);
  if (linked == 0) {
    GLint msg_size = 0;
    glGetProgramiv(res.native_handle(), GL_INFO_LOG_LENGTH, &msg_size);
    std::string msg;
    if (msg_size > 1) {
      msg.resize(msg_size);
      glGetProgramInfoLog(res.native_handle(), msg_size, nullptr, msg.data());
    } else
      msg = "unknown link error";
    throw std::runtime_error{"Faild to link GLSL program: " + msg};
  }
  return res;
}

shader_program::shader_program(shader_handle vertex, shader_handle fragment)
    : resource<shader_program_handle>{link(vertex, fragment)} {}

shader_program::shader_program(
    const char* vertex_shader_sources, const char* fragment_shader_sources)
    : shader_program{
          compile(gl::shader_type::vertex, vertex_shader_sources).get(),
          compile(gl::shader_type::fragment, fragment_shader_sources).get()} {}

} // namespace gl
