#pragma once

#include <concepts>
#include <span>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace vlk {

template <typename Vertex>
concept vertex_attributes = requires(Vertex v) {
  { Vertex::binding_description() } -> std::convertible_to<vk::VertexInputBindingDescription>;
  {
    Vertex::attribute_description()
  } -> std::convertible_to<std::span<const vk::VertexInputAttributeDescription>>;
};

template <vlk::vertex_attributes Vertex, std::integral Idx>
struct mesh_data_view {
  std::span<const Vertex> vertices;
  std::span<const Idx> indices;
};

template <vlk::vertex_attributes Vertex, std::integral Idx>
struct mesh_data {
  std::vector<Vertex> vertices;
  std::vector<Idx> indices;

  operator mesh_data_view<Vertex, Idx>() const noexcept { return {.vertices = vertices, .indices = indices}; }
};

} // namespace vlk
