#pragma once

#include <array>
#include <span>

#include <util/concat.hpp>
#include <util/unit.hpp>

#include <wayland/gles2/mesh_data.hpp>

class landscape {
public:
  explicit landscape(meters cell_radius, int columns, int rows);

  ~landscape() noexcept = default;

  landscape(const landscape &) = delete;
  landscape &operator=(const landscape &) = delete;
  landscape(landscape &&) = delete;
  landscape &operator=(landscape &&) = delete;

  [[nodiscard]] std::span<const vertex> verticies() const noexcept {
    return verticies_;
  }
  [[nodiscard]] std::span<const GLuint> indexes() const noexcept {
    return as_concated(as_concated(hexagons_));
  }

private:
  using triangle = std::array<GLuint, 3>;
  using hexagon = std::array<triangle, 6>;

  std::vector<vertex> verticies_;
  std::vector<hexagon> hexagons_;
};
