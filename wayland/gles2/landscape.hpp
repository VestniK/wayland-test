#pragma once

#include <gsl/span>

#include <wayland/gles2/mesh_data.hpp>

class landscape {
public:
  landscape(float cell_radius, int columns, int rows);

  landscape(const landscape&) = delete;
  landscape& operator=(const landscape&) = delete;
  landscape(landscape&&) = delete;
  landscape& operator=(landscape&&) = delete;

  gsl::span<const vertex> verticies() const noexcept { return verticies_; }
  gsl::span<const GLuint> indexes() const noexcept { return indexes_; }

private:
  std::vector<vertex> verticies_;
  std::vector<GLuint> indexes_;
};
