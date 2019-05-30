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
    return code(static_cast<unsigned>(pt.x), static_cast<unsigned>(pt.y));
  }
};

} // namespace morton
} // namespace

landscape::landscape(meters cell_radius, int columns, int rows) {
  std::unordered_map<triangular::point, GLuint, morton::hasher> idxs;
  auto idx = [&](triangular::point pt) {
    auto [it, success] = idxs.insert({pt, verticies_.size()});
    if (success) {
      verticies_.push_back(
          {{cell_radius.count() * triangular::to_cartesian(it->first), 0.},
              {0., 0., 1.}});
    }
    return it->second;
  };

  using namespace hexagon_net;

  for (int m = 0; m < columns; ++m) {
    for (int n = 0; n < rows - (m & 1); ++n) {
      triangular::point center = hexagon_net::cell_center(m, n);
      const GLuint c = idx(center);
      const GLuint bl = idx(cell_coord(center, corner::bottom_left));
      const GLuint ml = idx(cell_coord(center, corner::midle_left));
      const GLuint tl = idx(cell_coord(center, corner::top_left));
      const GLuint br = idx(cell_coord(center, corner::bottom_right));
      const GLuint mr = idx(cell_coord(center, corner::midle_right));
      const GLuint tr = idx(cell_coord(center, corner::top_right));

      hexagons_.push_back(
          {triangle{c, bl, ml}, triangle{c, ml, tl}, triangle{c, tl, tr},
              triangle{c, tr, mr}, triangle{c, mr, br}, triangle{c, br, bl}});
    }
  }
}
