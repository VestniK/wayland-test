#pragma once

#include <array>
#include <span>
#include <vector>

#include <mp-units/systems/si/si.h>

#include <libs/memtricks/concat.hpp>

#include <scene/mesh_data.hpp>

class landscape {
public:
  using radius_t =
      mp_units::quantity<mp_units::isq::radius[mp_units::si::metre], float>;

  explicit landscape(radius_t cell_radius, int columns, int rows);

  ~landscape() noexcept = default;

  landscape(const landscape&) = delete;
  landscape& operator=(const landscape&) = delete;
  landscape(landscape&&) = delete;
  landscape& operator=(landscape&&) = delete;

  [[nodiscard]] std::span<const vertex> verticies() const noexcept {
    return verticies_;
  }
  [[nodiscard]] std::span<const unsigned> indexes() const noexcept {
    return as_concated(as_concated(hexagons_));
  }

private:
  using triangle = std::array<unsigned, 3>;
  using hexagon = std::array<triangle, 6>;

  std::vector<vertex> verticies_;
  std::vector<hexagon> hexagons_;
};
