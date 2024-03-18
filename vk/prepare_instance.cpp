#include <vk/prepare_instance.hpp>

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <spdlog/spdlog.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

extern "C" {
extern const std::byte _binary_triangle_vert_spv_start[];
extern const std::byte _binary_triangle_vert_spv_end[];

extern const std::byte _binary_triangle_frag_spv_start[];
extern const std::byte _binary_triangle_frag_spv_end[];
}

namespace {

template <typename T, typename Cmp, typename... A>
constexpr auto make_sorted_array(Cmp&& cmp, A&&... a) {
  std::array<T, sizeof...(A)> res{T{std::forward<A>(a)}...};
  std::ranges::sort(res, cmp);
  return res;
}

constexpr std::array<const char*, 2> REQUIRED_EXTENSIONS{
    "VK_KHR_surface", "VK_KHR_wayland_surface"};

constexpr auto REQUIRED_DEVICE_EXTENSIONS = make_sorted_array<const char*>(
    std::less<std::string_view>{}, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

vk::raii::Instance create_instance() {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  vk::raii::Context context;

  vk::ApplicationInfo app_info{.pApplicationName = "wayland-test",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "no_engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0};
  const auto inst_create_info =
      vk::InstanceCreateInfo{.flags = {}, .pApplicationInfo = &app_info}
          .setPEnabledExtensionNames(REQUIRED_EXTENSIONS);
  vk::raii::Instance inst{context, inst_create_info, nullptr};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*inst);
  return inst;
}

vk::raii::Device create_logical_device(const vk::raii::PhysicalDevice& dev,
    uint32_t graphics_queue_family, uint32_t presentation_queue_family) {
  float queue_prio = 1.0f;
  std::array<vk::DeviceQueueCreateInfo, 2> device_queues{
      vk::DeviceQueueCreateInfo{.flags = {},
          .queueFamilyIndex = graphics_queue_family,
          .queueCount = 1,
          .pQueuePriorities = &queue_prio},
      vk::DeviceQueueCreateInfo{.flags = {},
          .queueFamilyIndex = presentation_queue_family,
          .queueCount = 1,
          .pQueuePriorities = &queue_prio}};
  vk::DeviceCreateInfo device_create_info{.flags = {},
      .queueCreateInfoCount = graphics_queue_family == presentation_queue_family
                                  ? 1u
                                  : 2u, // TODO: better duplication needed
      .pQueueCreateInfos = device_queues.data(),
      .enabledExtensionCount = REQUIRED_DEVICE_EXTENSIONS.size(),
      .ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data()};

  vk::raii::Device device{dev, device_create_info};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
  return device;
}

std::vector<vk::raii::ImageView> make_image_views(const vk::raii::Device& dev,
    std::span<const vk::Image> images, vk::Format fmt) {
  std::vector<vk::raii::ImageView> res;
  res.reserve(images.size());
  for (const auto& img : images) {
    vk::ImageViewCreateInfo inf{.image = img,
        .viewType = vk::ImageViewType::e2D,
        .format = fmt,
        .components = {.r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity},
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1}};
    res.push_back(vk::raii::ImageView{dev, inf});
  }
  return res;
}

class render_environment {
public:
  render_environment(const vk::raii::PhysicalDevice& dev,
      uint32_t graphics_queue_family, uint32_t presentation_queue_family,
      vk::SwapchainCreateInfoKHR swapchain_info)
      : device{create_logical_device(
            dev, graphics_queue_family, presentation_queue_family)},
        graphics_queue{device.getQueue(graphics_queue_family, 0)},
        presentation_queue{device.getQueue(presentation_queue_family, 0)},
        swapchain{device, swapchain_info},
        swapchain_images{swapchain.getImages()},
        image_views{make_image_views(
            device, swapchain_images, swapchain_info.imageFormat)},
        swapchain_image_format{swapchain_info.imageFormat} {}

  const vk::raii::Device& get_device() const noexcept { return device; }

  vk::Format get_image_format() const noexcept {
    return swapchain_image_format;
  }

private:
  vk::raii::Device device;
  vk::raii::Queue graphics_queue;
  vk::raii::Queue presentation_queue;
  vk::raii::SwapchainKHR swapchain;
  std::vector<vk::Image> swapchain_images;
  std::vector<vk::raii::ImageView> image_views;
  vk::Format swapchain_image_format;
};

bool has_required_extensions(const vk::PhysicalDevice& dev) {
  spdlog::debug("Checking required extensions on suitable device {}",
      std::string_view{dev.getProperties().deviceName});

  auto exts = dev.enumerateDeviceExtensionProperties();
  std::ranges::sort(exts,
      [](const vk::ExtensionProperties& l, const vk::ExtensionProperties& r) {
        return l.extensionName < r.extensionName;
      });

  bool has_missing_exts = false;
  std::span<const char* const> remaining_required{REQUIRED_DEVICE_EXTENSIONS};
  for (const vk::ExtensionProperties& ext : exts) {
    std::string_view name{ext.extensionName};
    if (remaining_required.empty() || name < remaining_required.front())
      continue;

    while (
        !remaining_required.empty() && !(name < remaining_required.front())) {
      if (name != remaining_required.front()) {
        has_missing_exts = true;
        spdlog::debug(
            "\tMissing required extension {}", remaining_required.front());
      }
      remaining_required = remaining_required.subspan(1);
    }
  }
  for (auto missing : remaining_required) {
    spdlog::debug("\tMissing required extension {}", missing);
    has_missing_exts = true;
  }
  return !has_missing_exts;
}

std::optional<vk::SwapchainCreateInfoKHR> choose_swapchain_params(
    const vk::PhysicalDevice& dev, const vk::SurfaceKHR& surf,
    vk::Extent2D sz) {
  const auto capabilities = dev.getSurfaceCapabilitiesKHR(surf);
  const auto formats = dev.getSurfaceFormatsKHR(surf);
  const auto modes = dev.getSurfacePresentModesKHR(surf);

  const auto fmt_it =
      std::ranges::find_if(formats, [](vk::SurfaceFormatKHR fmt) {
        return fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
               (fmt.format == vk::Format::eR8G8B8A8Srgb ||
                   fmt.format == vk::Format::eB8G8R8A8Srgb);
      });
  if (fmt_it == formats.end())
    return std::nullopt;

  // TODO: choose eFifo if it's available while eMailbox is not
  const auto mode_it = std::ranges::find(modes, vk::PresentModeKHR::eMailbox);
  if (mode_it == modes.end())
    return std::nullopt;

  return vk::SwapchainCreateInfoKHR{.flags = {},
      .surface = surf,
      .minImageCount = {},
      .imageFormat = fmt_it->format,
      .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
      .imageExtent = vk::Extent2D{.width = std::clamp(sz.width,
                                      capabilities.minImageExtent.width,
                                      capabilities.maxImageExtent.width),
          .height = std::clamp(sz.height, capabilities.minImageExtent.height,
              capabilities.maxImageExtent.height)},
      .imageArrayLayers = {},
      .imageUsage = {},
      .imageSharingMode = vk::SharingMode::eExclusive,
      .queueFamilyIndexCount = {},
      .pQueueFamilyIndices = {},
      .preTransform = capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = *mode_it,
      .clipped = true,
      .oldSwapchain = {}};
}

render_environment setup_suitable_device(const vk::raii::Instance& inst,
    const vk::SurfaceKHR& surf, vk::Extent2D sz) {
  auto devices = inst.enumeratePhysicalDevices();

  for (const vk::raii::PhysicalDevice& dev : inst.enumeratePhysicalDevices()) {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> presentation_family;
    for (const auto& [idx, queue_family] :
        std::views::enumerate(dev.getQueueFamilyProperties())) {
      if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
        graphics_family = idx;
      if (dev.getSurfaceSupportKHR(idx, surf))
        presentation_family = idx;
    }
    if (graphics_family && presentation_family &&
        has_required_extensions(*dev)) {
      if (auto swapchain_info = choose_swapchain_params(*dev, surf, sz)) {
        spdlog::info("Using Vulkan device: {}",
            std::string_view{dev.getProperties().deviceName});

        // TODO: get rid of this strange patching
        std::array<uint32_t, 2> queues{*graphics_family, *presentation_family};
        if (graphics_family != presentation_family) {
          swapchain_info->imageSharingMode = vk::SharingMode::eConcurrent;
          swapchain_info->queueFamilyIndexCount = queues.size();
          swapchain_info->pQueueFamilyIndices = queues.data();
        }

        return {dev, *graphics_family, *presentation_family, *swapchain_info};
      }
    }

    spdlog::debug("Vulkan device '{}' is rejected",
        std::string_view{dev.getProperties().deviceName});
  }

  throw std::runtime_error{"No suitable Vulkan device found"};
}

vk::raii::ShaderModule load_shader(
    const vk::raii::Device& dev, std::span<const std::byte> data) {
  return vk::raii::ShaderModule{
      dev, vk::ShaderModuleCreateInfo{.codeSize = data.size(),
               .pCode = reinterpret_cast<const uint32_t*>(data.data())}};
}

vk::raii::RenderPass make_render_pass(
    const vk::raii::Device& dev, vk::Format img_fmt) {
  vk::AttachmentDescription attachment_desc{.format = img_fmt,
      .samples = vk::SampleCountFlagBits::e1,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
      .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
      .initialLayout = vk::ImageLayout::eUndefined,
      .finalLayout = vk::ImageLayout::ePresentSrcKHR};

  vk::AttachmentReference attachement_ref{
      .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};
  vk::SubpassDescription subpass{
      .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .colorAttachmentCount = 1,
      .pColorAttachments = &attachement_ref,
      .pResolveAttachments = 0,
      .pDepthStencilAttachment = nullptr,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr};

  return vk::raii::RenderPass{
      dev, vk::RenderPassCreateInfo{.attachmentCount = 1,
               .pAttachments = &attachment_desc,
               .subpassCount = 1,
               .pSubpasses = &subpass,
               .dependencyCount = 0,
               .pDependencies = nullptr}};
}

vk::raii::Pipeline make_pipeline(const vk::raii::Device& dev,
    const vk::RenderPass& render_pass,
    const vk::PipelineLayout& pipeline_layout, vk::Extent2D swapchain_extent) {
  vk::raii::ShaderModule vert_mod = load_shader(
      dev, {_binary_triangle_vert_spv_start, _binary_triangle_vert_spv_end});
  vk::raii::ShaderModule frag_mod = load_shader(
      dev, {_binary_triangle_frag_spv_start, _binary_triangle_frag_spv_end});

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
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = nullptr,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = nullptr};

  vk::PipelineInputAssemblyStateCreateInfo input_info{
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = false};

  vk::Viewport viewport{.x = .0,
      .y = .0,
      .width = static_cast<float>(swapchain_extent.width),
      .height = static_cast<float>(swapchain_extent.height),
      .minDepth = .0,
      .maxDepth = 1.};
  vk::Rect2D scisors{.offset = {0, 0}, .extent = swapchain_extent};

  vk::PipelineViewportStateCreateInfo viewport_state{.viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scisors};

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
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
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
          .pDynamicState = nullptr,
          .layout = pipeline_layout,
          .renderPass = render_pass,
          .subpass = 0,
          .basePipelineHandle = nullptr,
          .basePipelineIndex = -1}};
}

} // namespace

void prepare_instance(wl_display& display, wl_surface& surf, size sz) {
  vk::raii::Instance inst = create_instance();
  vk::raii::SurfaceKHR vk_surf{
      inst, vk::WaylandSurfaceCreateInfoKHR{
                .flags = {}, .display = &display, .surface = &surf}};

  const vk::Extent2D swapchain_extent{.width = static_cast<uint32_t>(sz.width),
      .height = static_cast<uint32_t>(sz.height)};
  render_environment render_env =
      setup_suitable_device(inst, *vk_surf, swapchain_extent);

  vk::raii::PipelineLayout pipeline_layout{
      render_env.get_device(), vk::PipelineLayoutCreateInfo{
                                   .setLayoutCount = 0,
                                   .pSetLayouts = nullptr,
                                   .pushConstantRangeCount = 0,
                                   .pPushConstantRanges = nullptr,
                               }};

  vk::raii::RenderPass render_pass =
      make_render_pass(render_env.get_device(), render_env.get_image_format());

  vk::raii::Pipeline pipeline = make_pipeline(render_env.get_device(),
      *render_pass, *pipeline_layout, swapchain_extent);
}
