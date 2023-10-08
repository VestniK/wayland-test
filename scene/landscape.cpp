#include <algorithm>
#include <unordered_map>

#include <glm/ext.hpp>

#include <util/morton.hpp>

#include <scene/landscape.hpp>
#include <scene/triangular_net.hpp>

namespace {

struct morton_hash {
  constexpr size_t operator()(triangular::point pt) const noexcept {
    return morton::code(
        static_cast<unsigned>(pt.x), static_cast<unsigned>(pt.y));
  }
};

} // namespace

landscape::landscape(meters cell_radius, int columns, int rows) {
  std::unordered_map<triangular::point, unsigned, morton_hash> idxs;
  auto idx = [&](triangular::point pt) {
    auto [it, success] = idxs.insert({pt, verticies_.size()});
    if (success) {
      verticies_.push_back({.position = {cell_radius.count() *
                                             triangular::to_cartesian(
                                                 it->first),
                                0.},
          .normal = {0., 0., 1.},
          .uv = cell_radius.count() / 5 * triangular::to_cartesian(it->first)});
    }
    return it->second;
  };

  using namespace hexagon_net;

  for (int m = 0; m < columns; ++m) {
    for (int n = 0; n < rows - (m & 1); ++n) {
      triangular::point center = hexagon_net::cell_center(m, n);
      const unsigned c = idx(center);
      const unsigned bl = idx(cell_coord(center, corner::bottom_left));
      const unsigned ml = idx(cell_coord(center, corner::midle_left));
      const unsigned tl = idx(cell_coord(center, corner::top_left));
      const unsigned br = idx(cell_coord(center, corner::bottom_right));
      const unsigned mr = idx(cell_coord(center, corner::midle_right));
      const unsigned tr = idx(cell_coord(center, corner::top_right));

      hexagons_.push_back(
          {triangle{c, bl, ml}, triangle{c, ml, tl}, triangle{c, tl, tr},
              triangle{c, tr, mr}, triangle{c, mr, br}, triangle{c, br, bl}});
    }
  }
}
