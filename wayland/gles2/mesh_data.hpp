#pragma once

#include <GLES2/gl2.h>

#include <glm/glm.hpp>

#include <vector>

struct vertex {
  glm::vec3 position;
  glm::vec3 normal;
};

struct mesh_data {
  std::vector<vertex> verticies;
  std::vector<GLuint> indexes;
};
