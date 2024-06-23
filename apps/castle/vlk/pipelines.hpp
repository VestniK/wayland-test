#pragma once

#include <array>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <libs/memtricks/array.hpp>

namespace vlk {

template <typename Vertex>
concept vertex_attributes = requires(Vertex v) {
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

template <vertex_attributes V>
struct shaders {
  std::span<const std::byte> vertex;
  std::span<const std::byte> fragment;

  constexpr shader_sources sources() const noexcept {
    return {.vertex = vertex, .fragment = fragment};
  }
};

template <vk::ShaderStageFlagBits S, typename T>
struct uniform {
  using value_type = T;
  static constexpr uint32_t count = arity_v<T>;
  static constexpr vk::ShaderStageFlags stages = S;

  static constexpr vk::DescriptorSetLayoutBinding make_binding(
      uint32_t binding_idx) {
    return {
        .binding = binding_idx,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = count,
        .stageFlags = stages,
        .pImmutableSamplers = nullptr,
    };
  }
};

template <typename T>
using vertex_uniform = uniform<vk::ShaderStageFlagBits::eVertex, T>;
template <typename T>
using fragment_uniform = uniform<vk::ShaderStageFlagBits::eFragment, T>;
template <typename T>
using graphics_uniform = uniform<vk::ShaderStageFlagBits::eAllGraphics, T>;

template <typename... Ts>
class pipeline_bindings;

class pipeline_bindings_base {
public:
  const vk::DescriptorSetLayout& layout() const { return *bindings_layout_; }

protected:
  pipeline_bindings_base(vk::raii::DescriptorSetLayout layout)
      : bindings_layout_{std::move(layout)} {}

protected:
  vk::raii::DescriptorSetLayout bindings_layout_;
};

template <typename... Ts, vk::ShaderStageFlagBits... Ss>
class pipeline_bindings<uniform<Ss, Ts>...> : public pipeline_bindings_base {
public:
  pipeline_bindings(const vk::raii::Device& dev)
      : pipeline_bindings_base{
            make_layout(dev, std::index_sequence_for<Ts...>{})} {}

private:
  template <size_t... Is>
  static vk::raii::DescriptorSetLayout make_layout(
      const vk::raii::Device& dev, std::index_sequence<Is...>) {
    std::array<vk::DescriptorSetLayoutBinding, sizeof...(Ts)> bindings{
        {uniform<Ss, Ts>::make_binding(Is)...}};
    vk::DescriptorSetLayoutCreateInfo info{
        .bindingCount = bindings.size(), .pBindings = bindings.data()};
    return {dev, info};
  }
};

class pipelines_storage_base {
protected:
  pipelines_storage_base() noexcept = default;
  pipelines_storage_base(
      const vk::raii::Device& dev, const pipeline_bindings_base& bindings)
      : layout_{dev, vk::PipelineLayoutCreateInfo{
                         .setLayoutCount = 1,
                         .pSetLayouts = &bindings.layout(),
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

  template <vertex_attributes... Vs>
  pipelines_storage(const vk::raii::Device& dev, vk::RenderPass render_pass,
      const pipeline_bindings_base& bindings, shaders<Vs>... shaders)
      : pipelines_storage_base{dev, bindings},
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
