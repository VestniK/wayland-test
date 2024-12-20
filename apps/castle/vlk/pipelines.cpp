#include "pipelines.hpp"

namespace vlk {

namespace {

vk::raii::ShaderModule load_shader(
    const vk::raii::Device& dev, std::span<const std::byte> data) {
  return vk::raii::ShaderModule{
      dev, vk::ShaderModuleCreateInfo{.codeSize = data.size(),
               .pCode = reinterpret_cast<const uint32_t*>(data.data())}};
}

} // namespace

vk::raii::Pipeline pipelines_storage_base::make_pipeline(
    const vk::raii::Device& dev, vk::RenderPass render_pass,
    vk::SampleCountFlagBits samples, shader_sources shaders,
    const vk::VertexInputBindingDescription& vertex_binding,
    std::span<const vk::VertexInputAttributeDescription> vertex_attrs) {

  vk::raii::ShaderModule vert_mod = load_shader(dev, shaders.vertex);
  vk::raii::ShaderModule frag_mod = load_shader(dev, shaders.fragment);

  std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
      vk::PipelineShaderStageCreateInfo{
          .stage = vk::ShaderStageFlagBits::eVertex,
          .module = *vert_mod,
          .pName = "main"},
      vk::PipelineShaderStageCreateInfo{
          .stage = vk::ShaderStageFlagBits::eFragment,
          .module = *frag_mod,
          .pName = "main"}};

  vk::PipelineVertexInputStateCreateInfo vertex_input_info{
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_binding,
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(vertex_attrs.size()),
      .pVertexAttributeDescriptions = vertex_attrs.data()};

  vk::PipelineInputAssemblyStateCreateInfo input_info{
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = false};

  std::array<vk::DynamicState, 2> dyn_states = {
      vk::DynamicState::eViewport, vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dyn_state_info{
      .dynamicStateCount = dyn_states.size(),
      .pDynamicStates = dyn_states.data()};

  vk::PipelineViewportStateCreateInfo viewport_state{.viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr};

  vk::PipelineRasterizationStateCreateInfo rasterizer_info{
      .depthClampEnable = false,
      .rasterizerDiscardEnable = false,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eClockwise,
      .depthBiasEnable = false,
      .depthBiasConstantFactor = .0,
      .depthBiasClamp = .0,
      .depthBiasSlopeFactor = .0,
      .lineWidth = 1.};

  vk::PipelineMultisampleStateCreateInfo multisampling_info{
      .rasterizationSamples = samples,
      .sampleShadingEnable = false,
      .minSampleShading = 1.,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = false,
      .alphaToOneEnable = false};

  vk::PipelineColorBlendAttachmentState blend_attachment{
      .blendEnable = false,
      .srcColorBlendFactor = vk::BlendFactor::eOne,
      .dstColorBlendFactor = vk::BlendFactor::eZero,
      .colorBlendOp = vk::BlendOp::eAdd,
      .srcAlphaBlendFactor = vk::BlendFactor::eOne,
      .dstAlphaBlendFactor = vk::BlendFactor::eZero,
      .alphaBlendOp = vk::BlendOp::eAdd,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
  };
  vk::PipelineColorBlendStateCreateInfo color_blending{.logicOpEnable = false,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &blend_attachment,
      .blendConstants = std::array{.0f, .0f, .0f, .0f}};

  return vk::raii::Pipeline{dev, nullptr,
      vk::GraphicsPipelineCreateInfo{.stageCount = shader_stages.size(),
          .pStages = shader_stages.data(),
          .pVertexInputState = &vertex_input_info,
          .pInputAssemblyState = &input_info,
          .pTessellationState = nullptr,
          .pViewportState = &viewport_state,
          .pRasterizationState = &rasterizer_info,
          .pMultisampleState = &multisampling_info,
          .pDepthStencilState = nullptr,
          .pColorBlendState = &color_blending,
          .pDynamicState = &dyn_state_info,
          .layout = *layout_,
          .renderPass = render_pass,
          .subpass = 0,
          .basePipelineHandle = nullptr,
          .basePipelineIndex = -1}};
}

} // namespace vlk
