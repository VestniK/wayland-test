#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <libs/anime/clock.hpp>
#include <libs/memtricks/member.hpp>

#include <apps/castle/vlk/vertex.hpp>

namespace scene {

struct vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;

  constexpr static vk::VertexInputBindingDescription
  binding_description() noexcept {
    return vk::VertexInputBindingDescription{.binding = 0,
        .stride = sizeof(vertex),
        .inputRate = vk::VertexInputRate::eVertex};
  }

  constexpr static std::array<vk::VertexInputAttributeDescription, 3>
  attribute_description() noexcept {
    return {
        vk::VertexInputAttributeDescription{.location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = static_cast<uint32_t>(member_offset(&vertex::position))},
        vk::VertexInputAttributeDescription{.location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = static_cast<uint32_t>(member_offset(&vertex::normal))},
        vk::VertexInputAttributeDescription{.location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = static_cast<uint32_t>(member_offset(&vertex::uv))}};
  }
};

struct world_transformations {
  glm::mat4 camera;
  glm::mat4 model;
  glm::mat4 norm_rotation;
};

struct texture_transform {
  glm::mat4 models[5];
};

struct light_source {
  glm::vec3 pos;
  float intense;
  float ambient;
  float attenuation;
};

void update_world(
    frames_clock::time_point ts, world_transformations& world) noexcept;

glm::mat4 setup_camera(vk::Extent2D sz) noexcept;

vlk::mesh_data<vertex, uint16_t> make_paper();

} // namespace scene
