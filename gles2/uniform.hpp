#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <GLES2/gl2.h>

namespace gl {

template <typename T>
class uniform_location {
public:
  constexpr uniform_location() noexcept = default;
  constexpr explicit uniform_location(GLint location) noexcept
      : location_{location} {}

  // must be specialized for each reauired T
  void set_value(const T& val) = delete;

private:
  GLint location_ = 0;
};

template <>
inline void uniform_location<GLint>::set_value(const GLint& val) {
  glUniform1i(location_, val);
}

template <>
inline void uniform_location<float>::set_value(const float& val) {
  glUniform1f(location_, val);
}

template <>
inline void uniform_location<glm::mat4>::set_value(const glm::mat4& val) {
  glUniformMatrix4fv(location_, 1, GL_FALSE, glm::value_ptr(val));
}

template <>
inline void uniform_location<glm::mat3>::set_value(const glm::mat3& val) {
  glUniformMatrix3fv(location_, 1, GL_FALSE, glm::value_ptr(val));
}

template <>
inline void uniform_location<glm::vec3>::set_value(const glm::vec3& val) {
  glUniform3fv(location_, 1, glm::value_ptr(val));
}

template <>
inline void uniform_location<glm::vec2>::set_value(const glm::vec2& val) {
  glUniform2fv(location_, 1, glm::value_ptr(val));
}

} // namespace gl
