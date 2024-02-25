#pragma once

#include <glm/glm.hpp>

#include <vector>

struct vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
  glm::vec2 idxs;
};
