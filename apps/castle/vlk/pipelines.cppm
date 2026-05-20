module;

#include <array>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

export module vlk.pipelines;

import vlk.vertex;

export namespace vlk {

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

namespace vlk {

namespace {

vk::raii::ShaderModule load_shader(const vk::raii::Device& dev, std::span<const std::byte> data) {
  return vk::raii::ShaderModule{
      dev, vk::ShaderModuleCreateInfo{}
               .setCodeSize(data.size())
               .setPCode(reinterpret_cast<const uint32_t*>(data.data()))
  };
}

} // namespace

vk::raii::Pipeline pipelines_storage_base::make_pipeline(
    const vk::raii::Device& dev, vk::RenderPass render_pass, vk::SampleCountFlagBits samples,
    shader_sources shaders, const vk::VertexInputBindingDescription& vertex_binding,
    std::span<const vk::VertexInputAttributeDescription> vertex_attrs
) {

  vk::raii::ShaderModule vert_mod = load_shader(dev, shaders.vertex);
  vk::raii::ShaderModule frag_mod = load_shader(dev, shaders.fragment);

  std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
      vk::PipelineShaderStageCreateInfo{}
          .setStage(vk::ShaderStageFlagBits::eVertex)
          .setModule(*vert_mod)
          .setPName("main"),
      vk::PipelineShaderStageCreateInfo{}
          .setStage(vk::ShaderStageFlagBits::eFragment)
          .setModule(*frag_mod)
          .setPName("main")
  };

  auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo{}
                               .setVertexBindingDescriptions(vertex_binding)
                               .setVertexAttributeDescriptions(vertex_attrs);
  auto input_info =
      vk::PipelineInputAssemblyStateCreateInfo{}.setTopology(vk::PrimitiveTopology::eTriangleList);

  std::array<vk::DynamicState, 2> dyn_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  auto dyn_state_info = vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dyn_states);
  auto viewport_state = vk::PipelineViewportStateCreateInfo{}.setViewportCount(1).setScissorCount(1);

  auto rasterizer_info = vk::PipelineRasterizationStateCreateInfo{}
                             .setFrontFace(vk::FrontFace::eClockwise)
                             .setCullMode(vk::CullModeFlagBits::eBack)
                             .setLineWidth(1.);

  auto multisampling_info =
      vk::PipelineMultisampleStateCreateInfo{}.setRasterizationSamples(samples).setMinSampleShading(1.);

  auto blend_attachment = vk::PipelineColorBlendAttachmentState{}
                              .setSrcColorBlendFactor(vk::BlendFactor::eOne)
                              .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                              .setColorWriteMask(
                                  vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
                              );

  auto color_blending =
      vk::PipelineColorBlendStateCreateInfo{}.setLogicOp(vk::LogicOp::eCopy).setAttachments(blend_attachment);

  return vk::raii::Pipeline{
      dev, nullptr,
      vk::GraphicsPipelineCreateInfo{}
          .setStages(shader_stages)
          .setPVertexInputState(&vertex_input_info)
          .setPInputAssemblyState(&input_info)
          .setPViewportState(&viewport_state)
          .setPRasterizationState(&rasterizer_info)
          .setPMultisampleState(&multisampling_info)
          .setPColorBlendState(&color_blending)
          .setPDynamicState(&dyn_state_info)
          .setLayout(*layout_)
          .setRenderPass(render_pass)
          .setBasePipelineIndex(-1)
  };
}

} // namespace vlk
