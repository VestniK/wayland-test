#include "prepare_instance.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <spdlog/spdlog.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <libs/memtricks/member.hpp>
#include <libs/memtricks/projections.hpp>

#include <apps/castle/vlk/buf.hpp>
#include <apps/castle/vlk/cmds.hpp>
#include <apps/castle/vlk/pipelines.hpp>
#include <apps/castle/vlk/presentation.hpp>
#include <apps/castle/vlk/uniforms.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

extern "C" {
extern const std::byte _binary_triangle_vert_spv_start[];
extern const std::byte _binary_triangle_vert_spv_end[];

extern const std::byte _binary_triangle_frag_spv_start[];
extern const std::byte _binary_triangle_frag_spv_end[];
}

namespace {

template <vlk::vertex_attributes Vertex, std::integral Idx>
struct mesh_data_view {
  std::span<const Vertex> vertices;
  std::span<const Idx> indices;
};

template <vlk::vertex_attributes Vertex, std::integral Idx>
struct mesh_data {
  std::vector<Vertex> vertices;
  std::vector<Idx> indices;

  operator mesh_data_view<Vertex, Idx>() const noexcept {
    return {.vertices = vertices, .indices = indices};
  }
};

namespace scene {

struct vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;

  constexpr static vk::VertexInputBindingDescription
  binding_description() noexcept {
    return vk::VertexInputBindingDescription{.binding = 0,
        .stride = sizeof(vertex),
        .inputRate = vk::VertexInputRate::eVertex};
  }

  constexpr static std::array<vk::VertexInputAttributeDescription, 3>
  attribute_description() noexcept {
    return {
        vk::VertexInputAttributeDescription{.location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = static_cast<uint32_t>(member_offset(&vertex::position))},
        vk::VertexInputAttributeDescription{.location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = static_cast<uint32_t>(member_offset(&vertex::normal))},
        vk::VertexInputAttributeDescription{.location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = static_cast<uint32_t>(member_offset(&vertex::uv))}};
  }
};

struct world_transformations {
  glm::mat4 camera;
  glm::mat4 model;
  glm::mat3 norm_rotation;
};

// TODO: device specific minUniformBufferOffsetAlignment should be guarantied in
// a better way then hardcode here
struct alignas(64) light_source {
  glm::vec3 pos;
  float intense;
  float ambient;
  float attenuation;
};

void update_world(
    frames_clock::time_point ts, world_transformations& world) noexcept {
  using namespace std::literals;
  const float phase =
      static_cast<float>(2 * M_PI * (ts.time_since_epoch() % 5s).count() /
                         float(frames_clock::duration{5s}.count()));
  const auto model =
      glm::translate(glm::mat4{1.}, glm::vec3{std::cos(phase), 0., 0.});
  world.model = model;
  world.norm_rotation = glm::transpose(glm::inverse(glm::mat3(model)));
}

static glm::mat4 setup_camera(vk::Extent2D sz) noexcept {
  constexpr auto camera_pos = glm::vec3{0., 0., 50.};
  constexpr auto camera_up_direction = glm::vec3{0., 1., 0.};
  constexpr auto camera_look_at = glm::vec3{10., 10., 0.};
  return glm::perspectiveFov<float>(
             M_PI / 6., sz.width, sz.height, 30.f, 60.f) *
         glm::lookAt(camera_pos, camera_look_at, camera_up_direction);
}

mesh_data<vertex, uint16_t> make_paper() {
  mesh_data<vertex, uint16_t> res;

  constexpr int x_segments = 170;
  constexpr int y_segments = 210;
  auto pos2idx = [](int x, int y) -> uint16_t { return y * x_segments + x; };

  for (int y = 0; y < y_segments; ++y) {
    for (int x = 0; x < x_segments; ++x) {
      res.vertices.push_back({.position = {0.1 * x, 0.1 * y, 0.},
          .normal = {0., 0., 1.},
          .uv = {0.1 * x, 0.1 * y}});
      if (y > 0 && x < x_segments - 1) {
        res.indices.push_back(pos2idx(x, y - 1));
        res.indices.push_back(pos2idx(x + 1, y - 1));
        res.indices.push_back(pos2idx(x, y));

        res.indices.push_back(pos2idx(x + 1, y - 1));
        res.indices.push_back(pos2idx(x + 1, y));
        res.indices.push_back(pos2idx(x, y));
      }
    }
  }
  return res;
}

constexpr vk::ClearValue clear_color{
    .color = {.float32 = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.75f}}};

} // namespace scene

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
          as_string_view<&vk::LayerProperties::layerName>)) {
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

class mesh {
public:
  mesh(vlk::memory_pools& pools, const vk::raii::Device& dev,
      const vk::Queue& transfer_queue, const vk::CommandBuffer& copy_cmd,
      mesh_data_view<scene::vertex, uint16_t> data)
      : vbo_{pools.prepare_buffer(dev, transfer_queue, copy_cmd,
            vlk::memory_pools::vbo, std::as_bytes(data.vertices))},
        ibo_{pools.prepare_buffer(dev, transfer_queue, copy_cmd,
            vlk::memory_pools::ibo, std::as_bytes(data.indices))},
        count_{data.indices.size()} {}

  const vk::Buffer& get_vbo() const noexcept { return *vbo_; }
  const vk::Buffer& get_ibo() const noexcept { return *ibo_; }
  const size_t size() const noexcept { return count_; }

private:
  vk::raii::Buffer vbo_;
  vk::raii::Buffer ibo_;
  size_t count_;
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
        render_pass_{make_render_pass(device_, swapchain_info.imageFormat)},
        render_target_{device_, presentation_queue_family, std::move(surf),
            swapchain_info, *render_pass_},
        descriptor_bindings_{device_, phydev_.getMemoryProperties()},
        pipelines_{device_, *render_pass_, descriptor_bindings_,
            vlk::shaders<scene::vertex>{
                .vertex = {_binary_triangle_vert_spv_start,
                    _binary_triangle_vert_spv_end},
                .fragment = {_binary_triangle_frag_spv_start,
                    _binary_triangle_frag_spv_end}}},
        cmd_buffs_{device_, graphics_queue_family},
        mempools_{device_, phydev_.getMemoryProperties(),
            {.vbo_capacity = 2 * 1024 * 1024,
                .ibo_capacity = 2 * 1024 * 1024,
                .staging_size = 2 * 1024 * 1024}},
        mesh_{mempools_, device_, cmd_buffs_.queue(), cmd_buffs_.front(),
            scene::make_paper()},
        render_finished_{device_, vk::SemaphoreCreateInfo{}},
        image_available_{device_, vk::SemaphoreCreateInfo{}},
        frame_done_{device_, vk::FenceCreateInfo{}} {
    std::get<0>(descriptor_bindings_.value(0)).camera =
        scene::setup_camera(swapchain_info.imageExtent);
    std::get<1>(descriptor_bindings_.value(0)) = {.pos = {2., 5., 15.},
        .intense = 0.8,
        .ambient = 0.4,
        .attenuation = 0.01};
  }

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
    const auto extent = as_extent(sz);

    auto params = device_rendering_params::choose(
        *phydev_, render_target_.surface(), extent);
    if (!params)
      throw std::runtime_error{
          "Restore swapchain params after resise have failed"};

    render_target_.resize(
        device_, *render_pass_, params->swapchain_info, extent);

    if (sz.height > 0 && sz.height > 0)
      std::get<0>(descriptor_bindings_.value(0)).camera =
          scene::setup_camera(extent);
  }

  void draw(frames_clock::time_point ts) override {
    draw(render_target_.start_frame(*image_available_), ts)
        .present(*render_finished_);

    // Wait untill rendering commands from the buffer are finished in order
    // to safely reuse the buffer on the next frame.
    if (auto ec = make_error_code(device_.waitForFences(
            {*frame_done_}, true, std::numeric_limits<uint64_t>::max())))
      throw std::system_error(ec, "vkWaitForFence");
    device_.resetFences({*frame_done_});
  }

private:
  vlk::render_target::frame draw(
      vlk::render_target::frame frame, frames_clock::time_point ts) const {
    scene::update_world(ts, std::get<0>(descriptor_bindings_.value(0)));
    submit_cmd_buf(record_cmd_buffer(cmd_buffs_.front(), frame.buffer()));
    return frame;
  }

  vk::CommandBuffer record_cmd_buffer(
      vk::CommandBuffer cmd, const vk::Framebuffer& fb) const {
    cmd.reset();

    cmd.begin(
        vk::CommandBufferBeginInfo{.flags = {}, .pInheritanceInfo = nullptr});
    cmd.beginRenderPass(
        vk::RenderPassBeginInfo{.renderPass = *render_pass_,
            .framebuffer = fb,
            .renderArea = {.offset = {0, 0}, .extent = render_target_.extent()},
            .clearValueCount = 1,
            .pClearValues = &scene::clear_color},
        vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines_[0]);

    cmd.setViewport(
        0, {vk::Viewport{.x = 0.,
               .y = 0.,
               .width = static_cast<float>(render_target_.extent().width),
               .height = static_cast<float>(render_target_.extent().height),
               .minDepth = 0.,
               .maxDepth = 1.}});
    cmd.setScissor(
        0, {vk::Rect2D{.offset = {0, 0}, .extent = render_target_.extent()}});

    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, &mesh_.get_vbo(), offsets);
    cmd.bindIndexBuffer(mesh_.get_ibo(), 0, vk::IndexType::eUint16);

    descriptor_bindings_.use(0, cmd, pipelines_.layout());
    cmd.drawIndexed(static_cast<uint32_t>(mesh_.size()), 1, 0, 0, 0);
    cmd.endRenderPass();
    cmd.end();
    return cmd;
  }

  void submit_cmd_buf(const vk::CommandBuffer& cmd) const {
    descriptor_bindings_.flush(0);

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
    cmd_buffs_.queue().submit(submit_infos, *frame_done_);
  }

private:
  vk::raii::Instance instance_;
  vk::raii::PhysicalDevice phydev_;
  vk::raii::Device device_;
  vk::raii::RenderPass render_pass_;
  vlk::render_target render_target_;

  vlk::pipeline_bindings<1, vlk::graphics_uniform<scene::world_transformations>,
      vlk::fragment_uniform<scene::light_source>>
      descriptor_bindings_;
  vlk::pipelines_storage<1> pipelines_;
  vlk::command_buffers<1> cmd_buffs_;
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
