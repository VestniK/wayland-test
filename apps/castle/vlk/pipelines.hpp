#pragma once

#include <array>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "vertex.hpp"

namespace vlk {

struct shader_sources {
  std::span<const std::byte> vertex;
  std::span<const std::byte> fragment;
};

template <vertex_attributes V>
struct shaders {
  std::span<const std::byte> vertex;
  std::span<const std::byte> fragment;

  constexpr shader_sources sources() const noexcept { return {.vertex = vertex, .fragment = fragment}; }
};

class pipelines_storage_base {
public:
  const vk::PipelineLayout layout() const noexcept { return *layout_; }

protected:
  pipelines_storage_base() noexcept = default;
  pipelines_storage_base(const vk::raii::Device& dev, const vk::DescriptorSetLayout& descriptors_layout)
      : layout_{dev, vk::PipelineLayoutCreateInfo{}.setSetLayouts(descriptors_layout)} {}

  vk::raii::Pipeline make_pipeline(
      const vk::raii::Device& dev, vk::RenderPass render_pass, vk::SampleCountFlagBits samples,
      shader_sources shaders, const vk::VertexInputBindingDescription& vertex_binding,
      std::span<const vk::VertexInputAttributeDescription> vertex_attrs
  );

protected:
  vk::raii::PipelineLayout layout_{nullptr};
};

template <size_t N>
class pipelines_storage : public pipelines_storage_base {
public:
  pipelines_storage() noexcept = default;

  template <vertex_attributes... Vs>
  pipelines_storage(
      const vk::raii::Device& dev, vk::RenderPass render_pass, vk::SampleCountFlagBits samples,
      const vk::DescriptorSetLayout& descriptors_layout, shaders<Vs>... shaders
  )
      : pipelines_storage_base{dev, descriptors_layout},
        pipelines_{{make_pipeline(
            dev, render_pass, samples, shaders.sources(), Vs::binding_description(),
            Vs::attribute_description()
        )...}} {
    static_assert(sizeof...(Vs) == N);
  }

  vk::Pipeline operator[](size_t idx) const noexcept { return *pipelines_[idx]; }
  constexpr size_t size() const noexcept { return N; }

private:
  std::array<vk::raii::Pipeline, N> pipelines_;
};

} // namespace vlk
