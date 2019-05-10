#pragma once

#include <glm/glm.hpp>

namespace triangular {

struct point {
  int tx;
  int ty;
};

constexpr bool operator==(point lhs, point rhs) noexcept {
  return lhs.tx == rhs.tx && lhs.ty == rhs.ty;
}

extern const glm::mat2 to_cartesian_transformation;

inline glm::vec2 to_cartesian(point pt) noexcept {
  return to_cartesian_transformation * glm::vec2{pt.tx, pt.ty};
}

} // namespace triangular
