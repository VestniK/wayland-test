#include <stdexcept>

#include <libs/gles2/resource.hpp>

namespace gl {

namespace {

inline void run_shader_compilation(shader& shdr) {
  glCompileShader(shdr.native_handle());
  GLint compiled;
  glGetShaderiv(shdr.native_handle(), GL_COMPILE_STATUS, &compiled);
  if (compiled == 0) {
    GLint msg_size;
    glGetShaderiv(shdr.native_handle(), GL_INFO_LOG_LENGTH, &msg_size);
    std::string msg;
    if (msg_size > 1) {
      msg.resize(msg_size);
      glGetShaderInfoLog(shdr.native_handle(), msg_size, nullptr, msg.data());
    } else
      msg = "unknown compiation error";
    throw std::runtime_error{"Failed to compile shader: " + msg};
  }
}

} // namespace

shader compile(shader_type type, const char* src) { return compile(type, std::span<const char*>{&src, 1}); }

shader compile(shader_type type, std::span<const char* const> srcs) {
  shader res{shader_handle{glCreateShader(static_cast<GLenum>(type))}};
  if (!res)
    throw std::runtime_error{"Failed to create shader"};
  glShaderSource(res.native_handle(), srcs.size(), srcs.data(), nullptr);
  run_shader_compilation(res);
  return res;
}

shader compile(shader_type type, std::string_view src) {
  shader res{shader_handle{glCreateShader(static_cast<GLenum>(type))}};
  if (!res)
    throw std::runtime_error{"Failed to create shader"};
  const char* text = src.data();
  GLint len = static_cast<GLint>(src.size());
  glShaderSource(res.native_handle(), 1, &text, &len);
  run_shader_compilation(res);
  return res;
}

resource<shader_program_handle> link(shader_handle vertex, shader_handle fragment) {
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

shader_program::shader_program(const char* vertex_shader_sources, const char* fragment_shader_sources)
    : shader_program{
          compile(gl::shader_type::vertex, vertex_shader_sources).get(),
          compile(gl::shader_type::fragment, fragment_shader_sources).get()
      } {}

shader_program::shader_program(
    std::span<const char* const> vertex_shader_sources, std::span<const char* const> fragment_shader_sources
)
    : shader_program{
          compile(gl::shader_type::vertex, vertex_shader_sources).get(),
          compile(gl::shader_type::fragment, fragment_shader_sources).get()
      } {}

shader_program::shader_program(
    std::string_view vertex_shader_sources, std::string_view fragment_shader_sources
)
    : shader_program{
          compile(gl::shader_type::vertex, vertex_shader_sources).get(),
          compile(gl::shader_type::fragment, fragment_shader_sources).get()
      } {}

} // namespace gl
