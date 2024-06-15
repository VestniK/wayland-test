#include "prepare_instance.hpp"
#include "vlk_buf.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <spdlog/spdlog.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <libs/memtricks/member.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

extern "C" {
extern const std::byte _binary_triangle_vert_spv_start[];
extern const std::byte _binary_triangle_vert_spv_end[];

extern const std::byte _binary_triangle_frag_spv_start[];
extern const std::byte _binary_triangle_frag_spv_end[];
}

namespace {

struct vertex {
  glm::vec2 pos;
  glm::vec3 color;

  constexpr static vk::VertexInputBindingDescription
  binding_description() noexcept {
    return vk::VertexInputBindingDescription{.binding = 0,
        .stride = sizeof(vertex),
        .inputRate = vk::VertexInputRate::eVertex};
  }

  constexpr static std::array<vk::VertexInputAttributeDescription, 2>
  attribute_description() noexcept {
    return {vk::VertexInputAttributeDescription{.location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = static_cast<uint32_t>(member_offset(&vertex::pos))},
        vk::VertexInputAttributeDescription{.location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = static_cast<uint32_t>(member_offset(&vertex::color))}};
  }
};

constexpr vertex vertices[] = {
    // clang-format off
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
    // clang-format on
};
constexpr uint16_t indices[] = {0, 1, 2, 2, 3, 0};

template <typename T, typename Cmp, typename... A>
constexpr auto make_sorted_array(Cmp&& cmp, A&&... a) {
  std::array<T, sizeof...(A)> res{T{std::forward<A>(a)}...};
  std::ranges::sort(res, cmp);
  return res;
}

constexpr std::array<const char*, 2> required_extensions{
    "VK_KHR_surface", "VK_KHR_wayland_surface"};

constexpr auto required_device_extensions =
    make_sorted_array<const char*>(std::less<std::string_view>{},
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE_4_EXTENSION_NAME);

vk::raii::Instance create_instance() {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  vk::raii::Context context;

  vk::ApplicationInfo app_info{.pApplicationName = "wayland-test",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "no_engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_2};
  auto inst_create_info = vk::InstanceCreateInfo{.pApplicationInfo = &app_info}
                              .setPEnabledExtensionNames(required_extensions);

#if !defined(NDEBUG)
  std::array<const char*, 1> debug_layers{"VK_LAYER_KHRONOS_validation"};
  if (std::ranges::contains(
          vk::enumerateInstanceLayerProperties(VULKAN_HPP_DEFAULT_DISPATCHER),
          std::string_view{debug_layers.front()},
          &vk::LayerProperties::layerName)) {
    inst_create_info.setPEnabledLayerNames(debug_layers);
  }
#endif

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
      .enabledExtensionCount = required_device_extensions.size(),
      .ppEnabledExtensionNames = required_device_extensions.data()};

  vk::raii::Device device{dev, device_create_info};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
  return device;
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

vk::raii::ShaderModule load_shader(
    const vk::raii::Device& dev, std::span<const std::byte> data) {
  return vk::raii::ShaderModule{
      dev, vk::ShaderModuleCreateInfo{.codeSize = data.size(),
               .pCode = reinterpret_cast<const uint32_t*>(data.data())}};
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

  const auto binding_desc = vertex::binding_description();
  const auto attr_desc = vertex::attribute_description();
  vk::PipelineVertexInputStateCreateInfo vertex_input_info{
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding_desc,
      .vertexAttributeDescriptionCount = attr_desc.size(),
      .pVertexAttributeDescriptions = attr_desc.data()};

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

constexpr vk::Extent2D as_extent(size sz) noexcept {
  return {.width = static_cast<uint32_t>(sz.width),
      .height = static_cast<uint32_t>(sz.height)};
}

bool has_required_extensions(const vk::PhysicalDevice& dev) {
  spdlog::debug("Checking required extensions on suitable device {}",
      std::string_view{dev.getProperties().deviceName});

  auto exts = dev.enumerateDeviceExtensionProperties();
  std::ranges::sort(exts,
      [](const vk::ExtensionProperties& l, const vk::ExtensionProperties& r) {
        return l.extensionName < r.extensionName;
      });

  bool has_missing_exts = false;
  std::span<const char* const> remaining_required{required_device_extensions};
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
      .minImageCount = std::min(capabilities.minImageCount + 1,
          capabilities.maxImageCount == 0 ? std::numeric_limits<uint32_t>::max()
                                          : capabilities.maxImageCount),
      .imageFormat = fmt_it->format,
      .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
      .imageExtent = vk::Extent2D{.width = std::clamp(sz.width,
                                      capabilities.minImageExtent.width,
                                      capabilities.maxImageExtent.width),
          .height = std::clamp(sz.height, capabilities.minImageExtent.height,
              capabilities.maxImageExtent.height)},
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .queueFamilyIndexCount = {},
      .pQueueFamilyIndices = {},
      .preTransform = capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
      .presentMode = *mode_it,
      .clipped = true,
      .oldSwapchain = {}};
}

struct device_rendering_params {
  struct {
    uint32_t graphics;
    uint32_t presentation;
  } queue_families;
  vk::SwapchainCreateInfoKHR swapchain_info;

  static std::optional<device_rendering_params> choose(
      const vk::PhysicalDevice& dev, const vk::SurfaceKHR& surf,
      vk::Extent2D sz) {
    if (dev.getProperties().apiVersion < VK_API_VERSION_1_2)
      return std::nullopt;
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
        has_required_extensions(dev)) {
      if (auto res = choose_swapchain_params(dev, surf, sz)
                         .transform([&](auto swapchain_info) {
                           return device_rendering_params{
                               .queue_families = {.graphics = *graphics_family,
                                   .presentation = *presentation_family},
                               .swapchain_info = swapchain_info};
                         })) {
        // TODO: get rid of this strange patching
        std::array<uint32_t, 2> queues{*graphics_family, *presentation_family};
        if (graphics_family != presentation_family) {
          res->swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
          res->swapchain_info.queueFamilyIndexCount = queues.size();
          res->swapchain_info.pQueueFamilyIndices = queues.data();
        } else {
          res->swapchain_info.queueFamilyIndexCount = 1;
          res->swapchain_info.pQueueFamilyIndices = queues.data();
        }

        return res;
      }
    }
    return std::nullopt;
  }
};

class swapchain_env {
public:
  class frame {
  public:
    constexpr frame() noexcept = default;

    frame(const vk::raii::Device& dev, vk::RenderPass render_pass,
        vk::Image image, vk::Extent2D extent, vk::Format fmt)
        : image_{image},
          view_{
              dev, vk::ImageViewCreateInfo{.image = image_,
                       .viewType = vk::ImageViewType::e2D,
                       .format = fmt,
                       .components = {.r = vk::ComponentSwizzle::eIdentity,
                           .g = vk::ComponentSwizzle::eIdentity,
                           .b = vk::ComponentSwizzle::eIdentity,
                           .a = vk::ComponentSwizzle::eIdentity},
                       .subresourceRange = {.aspectMask =
                                                vk::ImageAspectFlagBits::eColor,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}}},
          framebuf_{dev, vk::FramebufferCreateInfo{.renderPass = render_pass,
                             .attachmentCount = 1,
                             .pAttachments = &(*view_),
                             .width = extent.width,
                             .height = extent.height,
                             .layers = 1}} {}

    static std::vector<frame> from_images(const vk::raii::Device& dev,
        vk::RenderPass render_pass, std::span<const vk::Image> images,
        vk::Extent2D extent, vk::Format fmt) {
      std::vector<frame> res;
      res.reserve(images.size());
      for (vk::Image img : images)
        res.push_back({dev, render_pass, img, extent, fmt});
      return res;
    }

    vk::Image image() const noexcept { return image_; }
    vk::ImageView image_view() const noexcept { return *view_; }
    vk::Framebuffer frameuffer() const noexcept { return *framebuf_; }

  private:
    vk::Image image_;
    vk::raii::ImageView view_{nullptr};
    vk::raii::Framebuffer framebuf_{nullptr};
  };

  class available_framebuffer {
  public:
    available_framebuffer(vk::Framebuffer fb, uint32_t idx) noexcept
        : fb_{std::move(fb)}, idx_{idx} {}

    available_framebuffer(const available_framebuffer&) = delete;
    available_framebuffer& operator=(const available_framebuffer&) = delete;

    available_framebuffer(available_framebuffer&&) noexcept = default;
    available_framebuffer& operator=(
        available_framebuffer&&) noexcept = default;

    ~available_framebuffer() noexcept = default;

    const vk::Framebuffer& framebuffer() const noexcept { return fb_; }

    void present(const vk::SwapchainKHR& swpchain,
        const vk::Queue& presentation_queue,
        const vk::Semaphore& done_sem) const {
      const auto ec = make_error_code(presentation_queue.presentKHR(
          vk::PresentInfoKHR{.waitSemaphoreCount = 1,
              .pWaitSemaphores = &done_sem,
              .swapchainCount = 1,
              .pSwapchains = &swpchain,
              .pImageIndices = &idx_,
              .pResults = nullptr}));
      if (ec)
        throw std::system_error(ec, "vkQueuePresentKHR");
    }

  private:
    vk::Framebuffer fb_;
    uint32_t idx_;
  };

public:
  swapchain_env(const vk::raii::Device& device, const vk::SurfaceKHR& surf,
      const vk::SwapchainCreateInfoKHR& swapchain_info,
      const vk::RenderPass& render_pass)
      : swapchain_{device, swapchain_info},
        swapchain_image_format_{swapchain_info.imageFormat},
        frames_{frame::from_images(device, render_pass, swapchain_.getImages(),
            swapchain_info.imageExtent, swapchain_info.imageFormat)},
        swapchain_extent_{swapchain_info.imageExtent} {}

  void clear() noexcept {
    frames_.clear();
    swapchain_.clear();
  }

  vk::Format image_format() const noexcept { return swapchain_image_format_; }
  vk::Extent2D extent() const noexcept { return swapchain_extent_; }

  const vk::SwapchainKHR& swapchain() const noexcept { return *swapchain_; }

  available_framebuffer acqure_framebuffer(
      const vk::Semaphore& image_available) const {
    const auto [res, idx] = swapchain_.acquireNextImage(
        std::numeric_limits<uint32_t>::max(), image_available);
    auto ec = make_error_code(res);
    if (ec)
      throw std::system_error(ec, "vkAcquireNextImageKHR");
    assert(idx < frames_.size());
    return {frames_[idx].frameuffer(), idx};
  }

  void present(available_framebuffer&& fb, const vk::Queue& presentation_queue,
      const vk::Semaphore& done_sem) const {
    fb.present(*swapchain_, presentation_queue, done_sem);
  }

private:
  vk::raii::SwapchainKHR swapchain_;
  vk::Format swapchain_image_format_;
  std::vector<frame> frames_;
  vk::Extent2D swapchain_extent_;
};

class mesh {
public:
  mesh(vlk::memory_pools& pools, const vk::raii::Device& dev,
      const vk::Queue& transfer_queue, const vk::CommandBuffer& copy_cmd,
      std::span<const vertex> verticies, std::span<const uint16_t> indices)
      : vbo_{pools.prepare_buffer(dev, transfer_queue, copy_cmd,
            vlk::memory_pools::vbo, std::as_bytes(verticies))},
        ibo_{pools.prepare_buffer(dev, transfer_queue, copy_cmd,
            vlk::memory_pools::ibo, std::as_bytes(indices))} {}

  const vk::Buffer& get_vbo() const noexcept { return *vbo_; }
  const vk::Buffer& get_ibo() const noexcept { return *ibo_; }

private:
  vk::raii::Buffer vbo_;
  vk::raii::Buffer ibo_;
};

class render_environment : public renderer_iface {
public:
  render_environment(vk::raii::Instance inst, vk::raii::PhysicalDevice dev,
      vk::raii::SurfaceKHR surf, uint32_t graphics_queue_family,
      uint32_t presentation_queue_family,
      vk::SwapchainCreateInfoKHR swapchain_info)
      : instance_{std::move(inst)}, phydev_{std::move(dev)},
        device_{create_logical_device(
            phydev_, graphics_queue_family, presentation_queue_family)},
        graphics_queue_{device_.getQueue(graphics_queue_family, 0)},
        presentation_queue_{device_.getQueue(presentation_queue_family, 0)},
        surf_{std::move(surf)},
        render_pass_{make_render_pass(device_, swapchain_info.imageFormat)},
        swapchain_env_{device_, *surf_, swapchain_info, *render_pass_},
        pipeline_layout_{device_,
            vk::PipelineLayoutCreateInfo{
                .setLayoutCount = 0,
                .pSetLayouts = nullptr,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr,
            }},
        pipeline_{make_pipeline(device_, *render_pass_, *pipeline_layout_,
            swapchain_info.imageExtent)},
        cmd_pool_{device_,
            vk::CommandPoolCreateInfo{
                .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = graphics_queue_family}},
        cmd_buffs_{
            device_, vk::CommandBufferAllocateInfo{.commandPool = *cmd_pool_,
                         .level = vk::CommandBufferLevel::ePrimary,
                         .commandBufferCount = 1}},
        mempools_{device_, phydev_.getMemoryProperties(),
            {.vbo_capacity = 65536,
                .ibo_capacity = 65536,
                .staging_size = 65536}},
        mesh_{mempools_, device_, *graphics_queue_, *cmd_buffs_.front(),
            vertices, indices},
        render_finished_{device_, vk::SemaphoreCreateInfo{}},
        image_available_{device_, vk::SemaphoreCreateInfo{}},
        frame_done_{device_, vk::FenceCreateInfo{}} {}

  render_environment(const render_environment&) = delete;
  render_environment& operator=(const render_environment&) = delete;

  render_environment(render_environment&&) noexcept = default;
  render_environment& operator=(render_environment&&) noexcept = default;

  ~render_environment() noexcept {
    if (*device_ != nullptr)
      device_.waitIdle();
  }

  void resize(size sz) override {
    device_.waitIdle();

    swapchain_env_.clear();

    auto params =
        device_rendering_params::choose(*phydev_, *surf_, as_extent(sz));
    if (!params)
      throw std::runtime_error{
          "Restore swapchain params after resise have failed"};

    // render_pass is keept untoched but was created with old image format
    // choosin new format might be not safe
    assert(swapchain_env_.image_format() == params->swapchain_info.imageFormat);
    swapchain_env_ =
        swapchain_env{device_, *surf_, params->swapchain_info, *render_pass_};
  }

  void draw(frames_clock::time_point ts) override { draw_frame(ts); }

  void draw_frame(frames_clock::time_point ts [[maybe_unused]]) const {
    auto fb = swapchain_env_.acqure_framebuffer(*image_available_);

    record_cmd_buffer(*cmd_buffs_.front(), fb.framebuffer());
    submit_cmd_buf(*cmd_buffs_.front());

    swapchain_env_.present(
        std::move(fb), *presentation_queue_, *render_finished_);
  }

private:
  void record_cmd_buffer(
      const vk::CommandBuffer& cmd, const vk::Framebuffer& fb) const {
    cmd.reset();

    cmd.begin(
        vk::CommandBufferBeginInfo{.flags = {}, .pInheritanceInfo = nullptr});
    vk::ClearValue clear_val{
        .color = {.float32 = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.75f}}};
    cmd.beginRenderPass(
        vk::RenderPassBeginInfo{.renderPass = *render_pass_,
            .framebuffer = fb,
            .renderArea = {.offset = {0, 0}, .extent = swapchain_env_.extent()},
            .clearValueCount = 1,
            .pClearValues = &clear_val},
        vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);

    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, &mesh_.get_vbo(), offsets);
    cmd.bindIndexBuffer(mesh_.get_ibo(), 0, vk::IndexType::eUint16);

    cmd.drawIndexed(static_cast<uint32_t>(std::size(indices)), 1, 0, 0, 0);
    cmd.endRenderPass();
    cmd.end();
  }

  void submit_cmd_buf(const vk::CommandBuffer& cmd) const {
    const vk::PipelineStageFlags wait_stage =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    std::array<vk::SubmitInfo, 1> submit_infos{
        vk::SubmitInfo{.waitSemaphoreCount = 1,
            .pWaitSemaphores = &*image_available_,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*render_finished_}};
    graphics_queue_.submit(submit_infos, *frame_done_);
    if (auto ec = make_error_code(device_.waitForFences(
            {*frame_done_}, true, std::numeric_limits<uint64_t>::max())))
      throw std::system_error(ec, "vkWaitForFence");
    device_.resetFences({*frame_done_});
  }

private:
  vk::raii::Instance instance_;
  vk::raii::PhysicalDevice phydev_;
  vk::raii::Device device_;
  vk::raii::Queue graphics_queue_;
  vk::raii::Queue presentation_queue_;
  vk::raii::SurfaceKHR surf_;
  vk::raii::RenderPass render_pass_;
  swapchain_env swapchain_env_;
  vk::raii::PipelineLayout pipeline_layout_;
  vk::raii::Pipeline pipeline_;
  vk::raii::CommandPool cmd_pool_;
  vk::raii::CommandBuffers cmd_buffs_;

  vlk::memory_pools mempools_;
  mesh mesh_;

  vk::raii::Semaphore render_finished_;
  vk::raii::Semaphore image_available_;
  vk::raii::Fence frame_done_;
};

render_environment setup_suitable_device(
    vk::raii::Instance inst, vk::raii::SurfaceKHR surf, vk::Extent2D sz) {
  for (vk::raii::PhysicalDevice dev : inst.enumeratePhysicalDevices()) {
    if (auto params = device_rendering_params::choose(*dev, *surf, sz)) {
      spdlog::info("Using Vulkan device: {}",
          std::string_view{dev.getProperties().deviceName});
      return render_environment{std::move(inst), std::move(dev),
          std::move(surf), params->queue_families.graphics,
          params->queue_families.presentation, params->swapchain_info};
    }
    spdlog::debug("Vulkan device '{}' is rejected",
        std::string_view{dev.getProperties().deviceName});
  }

  throw std::runtime_error{"No suitable Vulkan device found"};
}

} // namespace

std::unique_ptr<renderer_iface> prepare_instance(
    wl_display& display, wl_surface& surf, size sz) {
  vk::raii::Instance inst = create_instance();
  vk::raii::SurfaceKHR vk_surf{
      inst, vk::WaylandSurfaceCreateInfoKHR{
                .flags = {}, .display = &display, .surface = &surf}};

  return std::make_unique<render_environment>(setup_suitable_device(
      std::move(inst), std::move(vk_surf), as_extent(sz)));
}
