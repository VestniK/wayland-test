#include <algorithm>
#include <map>

#include <glm/ext.hpp>

#include <wayland/gles2/landscape.hpp>

mesh_data generate_flat_landscape(float cell_radius, int columns, int rows) {
  /*
   *    l2 --- r2
   *   /         \
   * l1     c     r1
   *   \         /
   *    l0 --- r0
   *
   * m - cell column
   * n - cell line
   *     __
   *  __/11\__
   * /01\__/20\
   * \__/10\__/
   * /00\__/  \ <- m = 2, n = -1
   * \__/  \__/
   *
   * Triangle coords system basis:
   *
   *     ty
   *    /       angle between basis vectors = M_PI/3
   *   /___tx   basis vector length = cell_radius
   *
   * triangle coords for each hexagon vertex:
   *   l0: {m - n    , 2*n + m - 1}
   *   l1: {m - n - 1, 2*n + m    }
   *   l2: {m - n - 1, 2*n + m + 1}
   *    c: {m - n    , 2*n + m    }
   *   r0: {m - n + 1, 2*n + m - 1}
   *   r1: {m - n + 1, 2*n + m    }
   *   r2: {m - n    , 2*n + m + 1}
   */
  mesh_data res;

  // clang-format off
  const auto triangle_to_cartesian = cell_radius * glm::mat2{
      1., 0.,
      std::cos(M_PI / 3), std::sin(M_PI / 3)
  };
  // clang-format on

  std::map<std::pair<int, int>, GLuint> idxs;
  auto idx = [&](int tx, int ty) {
    auto [it, success] = idxs.insert({{tx, ty}, res.verticies.size()});
    if (success) {
      res.verticies.push_back(
          {{triangle_to_cartesian * glm::vec2{tx, ty}, 0.}, {0., 0., 1.}});
    }
    return it->second;
  };

  for (int n = 0; n < rows; ++n) {
    const int m0 = -n / 2;
    for (int m = m0; m < columns - m0; ++m) {
      const GLuint c = idx(m - n, 2 * n + m);
      const GLuint l0 = idx(m - n, 2 * n + m - 1);
      const GLuint l1 = idx(m - n - 1, 2 * n + m);
      const GLuint l2 = idx(m - n - 1, 2 * n + m + 1);
      const GLuint r0 = idx(m - n + 1, 2 * n + m - 1);
      const GLuint r1 = idx(m - n + 1, 2 * n + m);
      const GLuint r2 = idx(m - n, 2 * n + m + 1);

      res.indexes.push_back(c);
      res.indexes.push_back(l0);
      res.indexes.push_back(l1);

      res.indexes.push_back(c);
      res.indexes.push_back(l1);
      res.indexes.push_back(l2);

      res.indexes.push_back(c);
      res.indexes.push_back(l2);
      res.indexes.push_back(r2);

      res.indexes.push_back(c);
      res.indexes.push_back(r2);
      res.indexes.push_back(r1);

      res.indexes.push_back(c);
      res.indexes.push_back(r1);
      res.indexes.push_back(r0);

      res.indexes.push_back(c);
      res.indexes.push_back(r0);
      res.indexes.push_back(l0);
    }
  }

  return res;
}
