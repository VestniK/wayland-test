#pragma once

#include <array>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace vlk {

template <typename Vertex>
concept vertex = requires(Vertex v) {
  {
    Vertex::binding_description()
  } -> std::convertible_to<vk::VertexInputBindingDescription>;
  {
    Vertex::attribute_description()
  }
  -> std::convertible_to<std::span<const vk::VertexInputAttributeDescription>>;
};

struct shader_sources {
  std::span<const std::byte> vertex;
  std::span<const std::byte> fragment;
};

template <vertex V>
struct shaders {
  std::span<const std::byte> vertex;
  std::span<const std::byte> fragment;

  constexpr shader_sources sources() const noexcept {
    return {.vertex = vertex, .fragment = fragment};
  }
};

class pipelines_storage_base {
protected:
  pipelines_storage_base() noexcept = default;
  pipelines_storage_base(const vk::raii::Device& dev)
      : layout_{dev, vk::PipelineLayoutCreateInfo{
                         .setLayoutCount = 0,
                         .pSetLayouts = nullptr,
                         .pushConstantRangeCount = 0,
                         .pPushConstantRanges = nullptr,
                     }} {}

  vk::raii::Pipeline make_pipeline(const vk::raii::Device& dev,
      vk::RenderPass render_pass, shader_sources shaders,
      const vk::VertexInputBindingDescription& vertex_binding,
      std::span<const vk::VertexInputAttributeDescription> vertex_attrs);

protected:
  vk::raii::PipelineLayout layout_{nullptr};
};

template <size_t N>
class pipelines_storage : public pipelines_storage_base {
public:
  pipelines_storage() noexcept = default;

  template <vertex... Vs>
  pipelines_storage(const vk::raii::Device& dev, vk::RenderPass render_pass,
      shaders<Vs>... shaders)
      : pipelines_storage_base{dev},
        pipelines_{{make_pipeline(dev, render_pass, shaders.sources(),
            Vs::binding_description(), Vs::attribute_description())...}} {
    static_assert(sizeof...(Vs) == N);
  }

  vk::Pipeline operator[](size_t idx) const noexcept {
    return *pipelines_[idx];
  }
  constexpr size_t size() const noexcept { return N; }

private:
  std::array<vk::raii::Pipeline, N> pipelines_;
};

} // namespace vlk
