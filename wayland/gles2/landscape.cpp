#include <algorithm>
#include <unordered_map>

#include <glm/ext.hpp>

#include <wayland/gles2/landscape.hpp>
#include <wayland/gles2/triangular_net.hpp>

namespace {
namespace morton {

constexpr uint64_t shifts[] = {16, 8, 4, 2, 1};
constexpr uint64_t masks[] = {0x0000'ffff'0000'ffff, 0x00ff'00ff'00ff'00ff,
    0x0f0f'0f0f'0f0f'0f0f, 0x3333'3333'3333'3333, 0x5555'5555'5555'5555};

constexpr uint64_t interleave(uint32_t val) noexcept {
  uint64_t r = val;
  r = (r | (r << shifts[0])) & masks[0];
  r = (r | (r << shifts[1])) & masks[1];
  r = (r | (r << shifts[2])) & masks[2];
  r = (r | (r << shifts[3])) & masks[3];
  r = (r | (r << shifts[4])) & masks[4];
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

landscape::landscape(float cell_radius, int columns, int rows) {
  /*
   *    l2 --- r2
   *   /         \
   * l1     c     r1
   *   \         /
   *    l0 --- r0
   *
   * m - cell column
   * n - cell row
   *     __
   *  __/11\__
   * /01\__/21\
   * \__/10\__/
   * /00\__/20\
   * \__/  \__/
   */

  std::unordered_map<triangular::point, GLuint, morton::hasher> idxs;
  auto idx = [&](int tx, int ty) {
    auto [it, success] = idxs.insert({{tx, ty}, verticies_.size()});
    if (success) {
      verticies_.push_back(
          {{cell_radius * triangular::to_cartesian(it->first), 0.},
              {0., 0., 1.}});
    }
    return it->second;
  };

  for (int m = 0; m < columns; ++m) {
    for (int n = 0; n < rows - m % 2; ++n) {
      triangular::point center{
          2 * m - n - (m + 1) / 2, 2 * n + 1 - (m + 1) % 2};
      const GLuint c = idx(center.tx, center.ty);
      const GLuint l0 = idx(center.tx, center.ty - 1);
      const GLuint l1 = idx(center.tx - 1, center.ty);
      const GLuint l2 = idx(center.tx - 1, center.ty + 1);
      const GLuint r0 = idx(center.tx + 1, center.ty - 1);
      const GLuint r1 = idx(center.tx + 1, center.ty);
      const GLuint r2 = idx(center.tx, center.ty + 1);

      triangles_.push_back({c, l0, l1});
      triangles_.push_back({c, l1, l2});
      triangles_.push_back({c, l2, r2});
      triangles_.push_back({c, r2, r1});
      triangles_.push_back({c, r1, r0});
      triangles_.push_back({c, r0, l0});
    }
  }
}
