#pragma once

#include <GLES2/gl2.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <libs/memtricks/member.hpp>

namespace gl {

template <typename T>
class attrib_location {
public:
  constexpr attrib_location() noexcept = default;
  constexpr explicit attrib_location(GLint location) noexcept
      : location_{location} {}

  template <typename C>
  void set_pointer(member_ptr<C, T> member);

private:
  GLint location_ = 0;
};

template <>
template <typename C>
void attrib_location<glm::vec3>::set_pointer(member_ptr<C, glm::vec3> member) {
  glVertexAttribPointer(location_, 3, GL_FLOAT, GL_FALSE, sizeof(C),
      reinterpret_cast<const GLvoid*>(member_offset(member)));
  glEnableVertexAttribArray(location_);
}

template <>
template <typename C>
void attrib_location<glm::vec2>::set_pointer(member_ptr<C, glm::vec2> member) {
  glVertexAttribPointer(location_, 2, GL_FLOAT, GL_FALSE, sizeof(C),
      reinterpret_cast<const GLvoid*>(member_offset(member)));
  glEnableVertexAttribArray(location_);
}

} // namespace gl
