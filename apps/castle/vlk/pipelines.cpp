#include "pipelines.hpp"

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
