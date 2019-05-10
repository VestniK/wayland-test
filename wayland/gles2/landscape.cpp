#include <algorithm>
#include <unordered_map>

#include <glm/ext.hpp>

#include <wayland/gles2/landscape.hpp>
#include <wayland/gles2/triangular_net.hpp>

namespace {
namespace morton {

constexpr uint64_t S[] = {16, 8, 4, 2, 1};
constexpr uint64_t M[] = {0x0000'ffff'0000'ffff, 0x00ff'00ff'00ff'00ff,
    0x0f0f'0f0f'0f0f'0f0f, 0x3333'3333'3333'3333, 0x5555'5555'5555'5555};

constexpr uint64_t interleave(uint32_t val) noexcept {
  uint64_t r = val;
  r = (r | (r << S[0])) & M[0];
  r = (r | (r << S[1])) & M[1];
  r = (r | (r << S[2])) & M[2];
  r = (r | (r << S[3])) & M[3];
  r = (r | (r << S[4])) & M[4];
  return r;
}

constexpr uint64_t code(uint32_t x, uint32_t y) noexcept {
  return interleave(x) | (interleave(y) << 1);
}

struct hasher {
  constexpr size_t operator()(triangular::point pt) const noexcept {
    return code(static_cast<unsigned>(pt.tx), static_cast<unsigned>(pt.ty));
  }
};

} // namespace morton
} // namespace

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

  std::unordered_map<triangular::point, GLuint, morton::hasher> idxs;
  auto idx = [&](int tx, int ty) {
    auto [it, success] = idxs.insert({{tx, ty}, res.verticies.size()});
    if (success) {
      res.verticies.push_back(
          {{cell_radius * triangular::to_cartesian(it->first), 0.},
              {0., 0., 1.}});
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
