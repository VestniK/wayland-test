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

using point = glm::ivec2;

extern const glm::mat2 to_cartesian_transformation;

inline glm::vec2 to_cartesian(point pt) noexcept { return to_cartesian_transformation * pt; }

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
  return {2 * m - n - (m + (m < 0 ? 0 : 1)) / 2, 2 * n + (m & 1)};
}

/**
 *    tl --- tr
 *   /         \
 * ml     c     mr
 *   \         /
 *    bl --- br
 */
enum class corner { top_left, top_right, midle_left, midle_right, bottom_left, bottom_right };

constexpr triangular::point cell_coord(triangular::point center, corner point_corner) noexcept {
  triangular::point res = center;
  switch (point_corner) {
  case corner::top_left:
    res += triangular::point{-1, 1};
    break;
  case corner::top_right:
    res += triangular::point{0, 1};
    break;
  case corner::midle_left:
    res += triangular::point{-1, 0};
    break;
  case corner::midle_right:
    res += triangular::point{1, 0};
    break;
  case corner::bottom_left:
    res += triangular::point{0, -1};
    break;
  case corner::bottom_right:
    res += triangular::point{1, -1};
    break;
  }
  return res;
}

} // namespace hexagon_net
