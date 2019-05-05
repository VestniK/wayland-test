#include <algorithm>

#include <glm/ext.hpp>

#include <wayland/gles2/landscape.hpp>

mesh_data generate_flat_landscape(float cell_radius, unsigned cell_count) {
  mesh_data res;
  res.indexes.reserve(3 * 6 * cell_count);

  const float scaled_cos_pi_3 = cell_radius * std::cos(M_PI / 3);
  const float scaled_sin_pi_3 = cell_radius * std::sin(M_PI / 3);

  // clang-format off
  const glm::vec2 hexagon_pattern[] = {
    {0., 0.}, // center point
    {cell_radius, 0.},
    {-cell_radius, 0.},
    {scaled_cos_pi_3, scaled_sin_pi_3},
    {-scaled_cos_pi_3, scaled_sin_pi_3}
  };
  const GLuint pattern_idxs[] = {
    2, 0, 1,
    2, 0, 3,
    2, 3, 5,
    2, 5, 6,
    2, 6, 4,
    2, 4, 1,
  };
  const glm::vec2 shift = {0. , 2*scaled_sin_pi_3};
  // clang-format on
  res.verticies.reserve(std::size(hexagon_pattern) * cell_count + 2);
  res.verticies.push_back({{hexagon_pattern[3] - shift, 0}, {0., 0., 1.}});
  res.verticies.push_back({{hexagon_pattern[4] - shift, 0}, {0., 0., 1.}});
  for (unsigned cell = 0; cell < cell_count; ++cell) {
    std::transform(std::begin(hexagon_pattern), std::end(hexagon_pattern),
        std::back_inserter(res.verticies),
        [cell_shift = cell * shift](glm::vec2 point) {
          return vertex{{point + cell_shift, 0.}, {0., 0., 1.}};
        });
    std::transform(std::begin(pattern_idxs), std::end(pattern_idxs),
        std::back_inserter(res.indexes),
        [idx_shift = std::size(hexagon_pattern) * cell](
            GLuint idx) { return idx + idx_shift; });
  }

  return res;
}
