#pragma once

#include <array>
#include <span>
#include <unordered_map>
#include <vector>

#include <glm/ext.hpp>

#include <mp-units/systems/si.h>

#include <libs/geom/morton.hpp>
#include <libs/geom/triangular_net.hpp>
#include <libs/memtricks/concat.hpp>

template <typename Vertex>
class hexagon_tiles {
private:
  struct morton_hash {
    constexpr size_t operator()(triangular::point pt) const noexcept {
      return morton::code(static_cast<unsigned>(pt.x), static_cast<unsigned>(pt.y));
    }
  };

public:
  using radius_t = mp_units::quantity<mp_units::isq::radius[mp_units::si::metre], float>;

  template <typename F>
  static hexagon_tiles generate(radius_t cell_radius, int columns, int rows, F&& to_vertex) {
    hexagon_tiles res;
    std::unordered_map<triangular::point, unsigned, morton_hash> idxs;
    auto idx = [&](triangular::point pt) {
      auto [it, success] = idxs.insert({pt, res.verticies_.size()});
      if (success) {
        res.verticies_.push_back(to_vertex(
            cell_radius.numerical_value_in(mp_units::si::metre) * triangular::to_cartesian(it->first)
        ));
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

        res.hexagons_.push_back(
            {triangle{c, bl, ml}, triangle{c, ml, tl}, triangle{c, tl, tr}, triangle{c, tr, mr},
             triangle{c, mr, br}, triangle{c, br, bl}}
        );
      }
    }
    return res;
  }

  [[nodiscard]] std::span<const Vertex> verticies() const noexcept { return verticies_; }
  [[nodiscard]] std::span<const unsigned> indexes() const noexcept {
    return as_concated(as_concated(hexagons_));
  }

private:
  hexagon_tiles() noexcept = default;

private:
  using triangle = std::array<unsigned, 3>;
  using hexagon = std::array<triangle, 6>;

  std::vector<Vertex> verticies_;
  std::vector<hexagon> hexagons_;
};
