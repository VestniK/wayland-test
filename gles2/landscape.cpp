#include <algorithm>
#include <unordered_map>

#include <glm/ext.hpp>

#include <util/morton.hpp>

#include <gles2/landscape.hpp>
#include <gles2/triangular_net.hpp>

namespace {

struct morton_hash {
  constexpr size_t operator()(triangular::point pt) const noexcept {
    return morton::code(
        static_cast<unsigned>(pt.x), static_cast<unsigned>(pt.y));
  }
};

} // namespace

landscape::landscape(meters cell_radius, int columns, int rows) {
  std::unordered_map<triangular::point, GLuint, morton_hash> idxs;
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
