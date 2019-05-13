#pragma once

#include <array>

#include <gsl/span>

#include <wayland/gles2/mesh_data.hpp>

class landscape {
public:
  explicit landscape(float cell_radius, int columns, int rows);

  landscape(const landscape&) = delete;
  landscape& operator=(const landscape&) = delete;
  landscape(landscape&&) = delete;
  landscape& operator=(landscape&&) = delete;

  gsl::span<const vertex> verticies() const noexcept { return verticies_; }
  gsl::span<const GLuint> indexes() const noexcept {
    return {triangles_.data()->data(),
        static_cast<gsl::span<const GLuint>::index_type>(
            triangles_.data()->size() * triangles_.size())};
  }

private:
  std::vector<vertex> verticies_;
  std::vector<std::array<GLuint, 3>> triangles_;
};
