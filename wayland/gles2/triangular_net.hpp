#pragma once

#include <glm/glm.hpp>

/**
 * Triangle coords system basis:
 *
 *     ty
 *    /       angle between basis vectors = M_PI/3
 *   /___tx   basis vector length = cell_radius
 */
namespace triangular {

struct point {
  int tx;
  int ty;
};

constexpr point operator+(point l, point r) noexcept {
  return {l.tx + r.tx, l.ty + r.ty};
}
constexpr point operator-(point l, point r) noexcept {
  return {l.tx - r.tx, l.ty - r.ty};
}
constexpr point operator-(point pt) noexcept { return {-pt.tx, -pt.ty}; }

constexpr bool operator==(point lhs, point rhs) noexcept {
  return lhs.tx == rhs.tx && lhs.ty == rhs.ty;
}

extern const glm::mat2 to_cartesian_transformation;

inline glm::vec2 to_cartesian(point pt) noexcept {
  return to_cartesian_transformation * glm::vec2{pt.tx, pt.ty};
}

} // namespace triangular

namespace hexagon_net {

/**
 * @param m cell column
 * @param n cell row
 *     __
 *  __/11\__
 * /01\__/21\
 * \__/10\__/
 * /00\__/20\
 * \__/  \__/
 */
constexpr inline triangular::point cell_center(int m, int n) noexcept {
  return {2 * m - n - (m + 1) / 2, 2 * n + 1 - (m + 1) % 2};
}

} // namespace hexagon_net
