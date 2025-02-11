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

  constexpr static vk::VertexInputBindingDescription binding_description() noexcept {
    return vk::VertexInputBindingDescription{}
        .setStride(sizeof(vertex))
        .setInputRate(vk::VertexInputRate::eVertex);
  }

  constexpr static std::array<vk::VertexInputAttributeDescription, 3> attribute_description() noexcept {
    return {
        vk::VertexInputAttributeDescription{}
            .setLocation(0)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(static_cast<uint32_t>(member_offset(&vertex::position))),
        vk::VertexInputAttributeDescription{}
            .setLocation(1)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(static_cast<uint32_t>(member_offset(&vertex::normal))),
        vk::VertexInputAttributeDescription{}
            .setLocation(2)
            .setFormat(vk::Format::eR32G32Sfloat)
            .setOffset(static_cast<uint32_t>(member_offset(&vertex::uv))),
    };
  }
};

struct world_transformations {
  glm::mat4 camera;
  glm::mat4 model;
  glm::mat4 norm_rotation;
};

struct texture_transform {
  glm::mat4 models[6];
};

struct light_source {
  glm::vec3 pos;
  float intense;
  float ambient;
  float attenuation;
};

void update_world(frames_clock::time_point ts, world_transformations& world) noexcept;

class catapult {
public:
  catapult() noexcept = default;
  explicit catapult(glm::vec2 anchor_point) noexcept : anchor_point{anchor_point} {}

  catapult& move(float distance) noexcept {
    anchor_point.x += distance;
    wheel_angle += distance / wheel_radius;
    return *this;
  }

  catapult& turn_arm(float angle) noexcept {
    arm_position = std::clamp(arm_position + angle, -1.f, 1.f);
    return *this;
  }

  std::array<glm::mat4, 4> sprites_transformations() const noexcept;

  glm::mat4 front_wheel_transformation() const noexcept;
  glm::mat4 rear_wheel_transformation() const noexcept;
  glm::mat4 platform_transformation() const noexcept;
  glm::mat4 arm_transformation() const noexcept;

private:
  static constexpr float wheel_radius = 0.5;

private:
  glm::vec2 anchor_point;
  float wheel_angle = 0.;
  float arm_position = 0.;
};

glm::mat4 setup_camera(vk::Extent2D sz) noexcept;

vlk::mesh_data<vertex, uint16_t> make_paper();

} // namespace scene
