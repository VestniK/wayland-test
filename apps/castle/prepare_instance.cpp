#include "prepare_instance.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <spdlog/spdlog.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <libs/img/load.hpp>
#include <libs/img/text.hpp>
#include <libs/memtricks/member.hpp>
#include <libs/memtricks/projections.hpp>
#include <libs/memtricks/sorted_array.hpp>
#include <libs/sfx/sfx.hpp>

#include <apps/castle/vlk/buf.hpp>
#include <apps/castle/vlk/cmds.hpp>
#include <apps/castle/vlk/pipelines.hpp>
#include <apps/castle/vlk/presentation.hpp>
#include <apps/castle/vlk/uniforms.hpp>
#include <apps/castle/vlk/vertex.hpp>

#include "scene.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

extern "C" {
extern const std::byte _binary_triangle_vert_spv_start[];
extern const std::byte _binary_triangle_vert_spv_end[];

extern const std::byte _binary_triangle_frag_spv_start[];
extern const std::byte _binary_triangle_frag_spv_end[];
}

using namespace std::literals;

namespace {

constexpr vk::ClearValue clear_color = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 0.75f};

constexpr std::array<const char*, 2> required_extensions{
    VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
};

constexpr auto required_device_extensions = make_sorted_array<const char*>(
    std::less<std::string_view>{}, VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE_4_EXTENSION_NAME
);

vk::raii::Instance create_instance() {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  vk::raii::Context context;

  auto app_info = vk::ApplicationInfo{}
                      .setPApplicationName("wayland-test")
                      .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                      .setPEngineName("no_engine")
                      .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
                      .setApiVersion(VK_API_VERSION_1_2);
  auto inst_create_info =
      vk::InstanceCreateInfo{}.setPApplicationInfo(&app_info).setPEnabledExtensionNames(required_extensions);

#if !defined(NDEBUG)
  std::array<const char*, 1> debug_layers{"VK_LAYER_KHRONOS_validation"};
  if (std::ranges::contains(vk::enumerateInstanceLayerProperties(VULKAN_HPP_DEFAULT_DISPATCHER),
          std::string_view{debug_layers.front()}, as_string_view<&vk::LayerProperties::layerName>)) {
    inst_create_info.setPEnabledLayerNames(debug_layers);
  }
#endif

  vk::raii::Instance inst{context, inst_create_info, nullptr};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*inst);
  return inst;
}

vk::raii::Device create_logical_device(
    const vk::raii::PhysicalDevice& dev, uint32_t graphics_queue_family, uint32_t presentation_queue_family
) {
  const float queue_prio = 1.0f;
  std::array<vk::DeviceQueueCreateInfo, 2> device_queues{
      vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex(graphics_queue_family).setQueuePriorities(queue_prio),
      vk::DeviceQueueCreateInfo{}
          .setQueueFamilyIndex(presentation_queue_family)
          .setQueuePriorities(queue_prio),
  };

  constexpr auto features = vk::PhysicalDeviceFeatures{}.setSamplerAnisotropy(true);

  auto device_create_info = vk::DeviceCreateInfo{}
                                .setQueueCreateInfoCount(
                                    graphics_queue_family == presentation_queue_family ? 1u : 2u
                                ) // TODO: better duplication needed
                                .setPQueueCreateInfos(device_queues.data())
                                .setPEnabledExtensionNames(required_device_extensions)
                                .setPEnabledFeatures(&features);

  vk::raii::Device device{dev, device_create_info};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
  return device;
}

vk::raii::RenderPass
make_render_pass(const vk::raii::Device& dev, vk::Format img_fmt, vk::SampleCountFlagBits samples) {
  using enum vk::SampleCountFlagBits;

  std::array<vk::AttachmentDescription, 2> attachments{
      vk::AttachmentDescription{}
          .setFormat(img_fmt)
          .setSamples(samples)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal),
      vk::AttachmentDescription{}
          .setFormat(img_fmt)
          .setLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
          .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
  };

  std::array<vk::AttachmentReference, 2> refs{
      vk::AttachmentReference{}.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal),
      vk::AttachmentReference{}.setAttachment(1).setLayout(vk::ImageLayout::eColorAttachmentOptimal)
  };
  const auto subpass = vk::SubpassDescription{}.setColorAttachments(refs[0]).setResolveAttachments(refs[1]);

  return vk::raii::RenderPass{
      dev, vk::RenderPassCreateInfo{}.setAttachments(attachments).setSubpasses(subpass)
  };
}

constexpr vk::Extent2D as_extent(size sz) noexcept {
  return {static_cast<uint32_t>(sz.width), static_cast<uint32_t>(sz.height)};
}

bool has_required_extensions(vk::PhysicalDevice dev) {
  spdlog::debug(
      "Checking required extensions on suitable device {}", std::string_view{dev.getProperties().deviceName}
  );

  std::vector<vk::ExtensionProperties> exts = dev.enumerateDeviceExtensionProperties();
  std::ranges::sort(exts, std::less<>{}, &vk::ExtensionProperties::extensionName);

  std::vector<std::string_view> missing_exts;
  std::ranges::set_difference(
      required_device_extensions,
      exts | std::views::transform(as_string_view<&vk::ExtensionProperties::extensionName>),
      std::back_inserter(missing_exts), std::less<std::string_view>{}
  );
  for (auto ext : missing_exts)
    spdlog::debug("\tMissing required extension {}", ext);

  return missing_exts.empty();
}

bool check_device(vk::PhysicalDevice dev) {
  if (dev.getProperties().apiVersion < VK_API_VERSION_1_2)
    return false;
  if (!dev.getFeatures().samplerAnisotropy)
    return false;

  return has_required_extensions(dev);
}

bool check_suitable_presentation_params(vk::PhysicalDevice dev, vk::SurfaceKHR surf) {
  if (!std::ranges::any_of(dev.getSurfaceFormatsKHR(surf), [](vk::SurfaceFormatKHR fmt) {
        return fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
               (fmt.format == vk::Format::eR8G8B8A8Srgb || fmt.format == vk::Format::eB8G8R8A8Srgb);
      }))
    return false;

  if (!std::ranges::contains(dev.getSurfacePresentModesKHR(surf), vk::PresentModeKHR::eMailbox))
    return false;

  return true;
}

vk::SwapchainCreateInfoKHR
make_swapchain_create_info(vk::PhysicalDevice dev, vk::SurfaceKHR surf, vk::Format img_fmt, vk::Extent2D sz) {
  const auto capabilities = dev.getSurfaceCapabilitiesKHR(surf);
  return vk::SwapchainCreateInfoKHR{}
      .setSurface(surf)
      .setMinImageCount(std::min(
          capabilities.minImageCount + 1,
          capabilities.maxImageCount == 0 ? std::numeric_limits<uint32_t>::max() : capabilities.maxImageCount
      ))
      .setImageFormat(img_fmt)
      .setImageExtent(vk::Extent2D{
          std::clamp(sz.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
          std::clamp(sz.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
      })
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setPreTransform(capabilities.currentTransform)
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
      .setPresentMode(vk::PresentModeKHR::eMailbox)
      .setClipped(true);
}

struct device_queue_families {
  uint32_t graphics;
  uint32_t presentation;

  static std::optional<device_queue_families> find(vk::PhysicalDevice dev, vk::SurfaceKHR surf) {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> presentation_family;
    for (const auto& [idx, queue_family] : std::views::enumerate(dev.getQueueFamilyProperties())) {
      if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
        graphics_family = idx;
      if (dev.getSurfaceSupportKHR(idx, surf))
        presentation_family = idx;
    }
    if (!graphics_family || !presentation_family)
      return std::nullopt;
    return device_queue_families{.graphics = *graphics_family, .presentation = *presentation_family};
  }
};

class gpu_device {
public:
  gpu_device() noexcept = default;
  gpu_device(
      vk::raii::Instance&& inst, vk::raii::PhysicalDevice&& dev, device_queue_families families
  ) noexcept
      : instance_{std::move(inst)}, phydev_{std::move(dev)},
        device_{create_logical_device(phydev_, families.graphics, families.presentation)},
        families_{families} {}

  const vk::raii::Device& dev() const noexcept { return device_; }

  vk::PhysicalDeviceMemoryProperties memory_properties() const noexcept {
    return phydev_.getMemoryProperties();
  }
  vk::PhysicalDeviceLimits limits() const noexcept { return phydev_.getProperties().limits; }

  vk::SampleCountFlagBits find_max_usable_samples() const noexcept {
    using enum vk::SampleCountFlagBits;
    constexpr vk::SampleCountFlagBits prio_order[] = {e64, e32, e16, e8, e4, e2};

    const auto counts_mask = phydev_.getProperties().limits.framebufferColorSampleCounts;
    const auto it = std::ranges::find_if(prio_order, [&](auto e) { return bool{e & counts_mask}; });
    return it != std::end(prio_order) ? *it : e1;
  }

  device_queue_families families() const noexcept { return families_; }

  std::optional<vk::Format> find_compatible_format_for(vk::SurfaceKHR surf) const {
    const auto formats = phydev_.getSurfaceFormatsKHR(surf);
    const auto fmt_it = std::ranges::find_if(formats, [](vk::SurfaceFormatKHR fmt) {
      return fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
             (fmt.format == vk::Format::eR8G8B8A8Srgb || fmt.format == vk::Format::eB8G8R8A8Srgb);
    });
    if (fmt_it == formats.end())
      return std::nullopt;
    return fmt_it->format;
  }

  vk::SwapchainCreateInfoKHR
  make_swapchain_info(vk::SurfaceKHR surf, vk::Format img_fmt, vk::Extent2D sz) const {
    auto res = make_swapchain_create_info(*phydev_, surf, img_fmt, sz);
    // TODO: Get rid of this strange patching.
    // FIXME: This code returns pointers to local array :( !!! And always was.
    std::array<uint32_t, 2> queues{families_.graphics, families_.presentation};
    if (families_.graphics != families_.presentation) {
      res.imageSharingMode = vk::SharingMode::eConcurrent;
      res.queueFamilyIndexCount = queues.size();
      res.pQueueFamilyIndices = queues.data();
    } else {
      res.queueFamilyIndexCount = 1;
      res.pQueueFamilyIndices = queues.data();
    }
    return res;
  }

private:
  vk::raii::Instance instance_{nullptr};
  vk::raii::PhysicalDevice phydev_{nullptr};
  vk::raii::Device device_{nullptr};
  device_queue_families families_{};
};

class mesh {
public:
  mesh(
      vlk::memory_pools& pools, const vk::raii::Device& dev, const vk::Queue& transfer_queue,
      const vk::CommandBuffer& copy_cmd, vlk::mesh_data_view<scene::vertex, uint16_t> data
  )
      : vbo_{pools.prepare_buffer(
            dev, transfer_queue, copy_cmd, vlk::memory_pools::vbo,
            pools.allocate_staging_memory(std::as_bytes(data.vertices))
        )},
        ibo_{pools.prepare_buffer(
            dev, transfer_queue, copy_cmd, vlk::memory_pools::ibo,
            pools.allocate_staging_memory(std::as_bytes(data.indices))
        )},
        count_{data.indices.size()} {}

  const vk::Buffer& get_vbo() const noexcept { return *vbo_; }
  const vk::Buffer& get_ibo() const noexcept { return *ibo_; }
  const size_t size() const noexcept { return count_; }

private:
  vk::raii::Buffer vbo_;
  vk::raii::Buffer ibo_;
  size_t count_;
};

vk::raii::Image load_text_texture(
    vlk::memory_pools& pools, const vk::raii::Device& dev, vk::Queue transfer_queue, vk::CommandBuffer cmd,
    std::string_view text
) {
  auto resources = sfx::archive::open_self();
  auto& fd = resources.open("fonts/RuthlessSketch.ttf");
  auto font = img::font::load(fd);
  auto reader = font.text_image_reader(text);

  vk::Format fmt;
  switch (reader.format()) {
  case img::pixel_fmt::rgb:
    fmt = vk::Format::eR8G8B8Srgb;
    break;
  case img::pixel_fmt::rgba:
    fmt = vk::Format::eR8G8B8A8Srgb;
    break;
  case img::pixel_fmt::grayscale:
    fmt = vk::Format::eR8Srgb;
    break;
  }

  auto stage_mem = pools.allocate_staging_memory(reader.pixels_size());
  reader.read_pixels(stage_mem);

  return pools.prepare_image(
      dev, transfer_queue, cmd, vlk::image_purpose::texture, std::move(stage_mem), fmt,
      as_extent(reader.size())
  );
}

vk::raii::Image load_sfx_texture(
    vlk::memory_pools& pools, const vk::raii::Device& dev, vk::Queue transfer_queue, vk::CommandBuffer cmd,
    const fs::path& sfx_path
) {
  auto resources = sfx::archive::open_self();
  auto& fd = resources.open(sfx_path);
  auto reader = img::load_reader(fd);

  vk::Format fmt;
  switch (reader.format()) {
  case img::pixel_fmt::rgb:
    fmt = vk::Format::eR8G8B8Srgb;
    break;
  case img::pixel_fmt::rgba:
    fmt = vk::Format::eR8G8B8A8Srgb;
    break;
  case img::pixel_fmt::grayscale:
    fmt = vk::Format::eR8Srgb;
    break;
  }

  auto stage_mem = pools.allocate_staging_memory(reader.pixels_size());
  reader.read_pixels(stage_mem);

  return pools.prepare_image(
      dev, transfer_queue, cmd, vlk::image_purpose::texture, std::move(stage_mem), fmt,
      as_extent(reader.size())
  );
}

static vk::raii::Sampler make_sampler(const vk::raii::Device& dev, const vk::PhysicalDeviceLimits& limits) {
  return dev.createSampler(vk::SamplerCreateInfo{}
                               .setMagFilter(vk::Filter::eLinear)
                               .setMinFilter(vk::Filter::eLinear)
                               .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                               .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
                               .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
                               .setAddressModeW(vk::SamplerAddressMode::eClampToBorder)
                               .setAnisotropyEnable(true)
                               .setMaxAnisotropy(limits.maxSamplerAnisotropy));
}

static vk::raii::ImageView make_view(const vk::raii::Device& dev, vk::Image img) {
  return dev.createImageView(vk::ImageViewCreateInfo{}
                                 .setImage(img)
                                 .setViewType(vk::ImageViewType::e2D)
                                 .setFormat(vk::Format::eR8G8B8A8Srgb)
                                 .setSubresourceRange(vk::ImageSubresourceRange{}
                                                          .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                          .setLevelCount(1)
                                                          .setLayerCount(1)));
}

std::tuple<vk::raii::Image, vk::raii::ImageView>
make_image_view(const vk::raii::Device& dev, vk::raii::Image&& img) {
  auto view = make_view(dev, *img);
  return std::tuple{std::move(img), std::move(view)};
}

struct uniform_objects {
  uniform_objects(
      const vk::raii::Device& dev, const vk::PhysicalDeviceLimits& limits, std::array<vk::raii::Image, 4> img,
      vk::raii::Image fwheel, vk::raii::Image rwheel, vk::raii::Image platform, vk::raii::Image arm,
      vk::raii::Image word_img
  )
      : sampler{make_sampler(dev, limits)}, castle_textures{std::move(img)},
        castle_texture_view{make_view(dev, *castle_textures[0])},
        catapult{
            make_image_view(dev, std::move(fwheel)), make_image_view(dev, std::move(rwheel)),
            make_image_view(dev, std::move(platform)), make_image_view(dev, std::move(arm))
        },
        word{std::move(word_img)}, word_view{make_view(dev, *word)} {}

  vlk::ubo::unique_ptr<scene::world_transformations> world;
  vlk::ubo::unique_ptr<scene::light_source> light;
  vlk::ubo::unique_ptr<scene::texture_transform> transformations;

  vk::raii::Sampler sampler;
  std::array<vk::raii::Image, 4> castle_textures;
  vk::raii::ImageView castle_texture_view;

  std::array<std::tuple<vk::raii::Image, vk::raii::ImageView>, 4> catapult;
  vk::raii::Image word;
  vk::raii::ImageView word_view;

  void switch_castle_image(const vk::raii::Device& dev, size_t n) {
    castle_texture_view = make_view(dev, *castle_textures[n]);
  }

  void bind(vlk::ubo_builder& bldr) {
    std::array<vk::ImageView, 6> sprites{*castle_texture_view,      *std::get<1>(catapult[0]),
                                         *std::get<1>(catapult[1]), *std::get<1>(catapult[2]),
                                         *std::get<1>(catapult[3]), *word_view};

    world = bldr.create<vlk::graphics_uniform<scene::world_transformations>>(0);
    light = bldr.create<vlk::fragment_uniform<scene::light_source>>(1);
    bldr.bind<vlk::fragment_uniform<vk::Sampler>>(2, *sampler);
    transformations = bldr.create<vlk::fragment_uniform<scene::texture_transform>>(3);
    bldr.bind<vlk::fragment_uniform<vk::ImageView[6]>>(4, sprites);
  }

  void rebind(vlk::ubo_builder& bldr) {
    std::array<vk::ImageView, 6> sprites{*castle_texture_view,      *std::get<1>(catapult[0]),
                                         *std::get<1>(catapult[1]), *std::get<1>(catapult[2]),
                                         *std::get<1>(catapult[3]), *word_view};

    bldr.bind<vlk::graphics_uniform<scene::world_transformations>>(0, *world);
    bldr.bind<vlk::fragment_uniform<scene::light_source>>(1, *light);
    bldr.bind<vlk::fragment_uniform<vk::Sampler>>(2, *sampler);
    bldr.bind<vlk::fragment_uniform<scene::texture_transform>>(3, *transformations);
    bldr.bind<vlk::fragment_uniform<vk::ImageView[6]>>(4, sprites);
  }
};

class render_environment : public renderer_iface {
public:
  render_environment(
      gpu_device gpu, vk::raii::SurfaceKHR surf, device_queue_families families,
      vk::SwapchainCreateInfoKHR swapchain_info
  )
      : gpu_{std::move(gpu)},
        uniform_pools_{
            gpu_.dev(), gpu_.memory_properties(), gpu_.limits(),
            vlk::descriptor_pool_builder{}
                .add_binding<
                    vlk::graphics_uniform<scene::world_transformations>,
                    vlk::fragment_uniform<scene::light_source>, vlk::fragment_uniform<vk::Sampler>,
                    vlk::fragment_uniform<scene::texture_transform>, vlk::fragment_uniform<vk::ImageView[6]>>(
                    1
                )
                .build(gpu_.dev(), 1),
            128 * 1024
        },
        mempools_{
            gpu_.dev(),
            gpu_.memory_properties(),
            gpu_.limits(),
            {.vbo_capacity = 2 * 1024 * 1024,
             .ibo_capacity = 2 * 1024 * 1024,
             .textures_capacity = 256 * 1024 * 1024,
             .staging_size = 64 * 1024 * 1024}
        },
        render_pass_{make_render_pass(gpu_.dev(), swapchain_info.imageFormat, gpu_.find_max_usable_samples())
        },
        render_target_{gpu_.dev(),
                       gpu_.memory_properties(),
                       families.presentation,
                       std::move(surf),
                       swapchain_info,
                       *render_pass_,
                       gpu_.find_max_usable_samples()},
        cmd_buffs_{gpu_.dev(), families.graphics},
        uniforms_{
            gpu_.dev(),
            gpu_.limits(),
            {load_sfx_texture(
                 mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/castle-0hit.png"
             ),
             load_sfx_texture(
                 mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/castle-1hit.png"
             ),
             load_sfx_texture(
                 mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/castle-2hit.png"
             ),
             load_sfx_texture(
                 mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/castle-3hit.png"
             )},
            load_sfx_texture(
                mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(),
                "textures/catapult-front-wheel.png"
            ),
            load_sfx_texture(
                mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(),
                "textures/catapult-rear-wheel.png"
            ),
            load_sfx_texture(
                mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(),
                "textures/catapult-platform.png"
            ),
            load_sfx_texture(
                mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/catapult-arm.png"
            ),
            load_text_texture(mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(), "привет")
        },
        descriptor_bindings_{uniform_pools_.make_pipeline_bindings<
            uniform_objects, 1, vlk::graphics_uniform<scene::world_transformations>,
            vlk::fragment_uniform<scene::light_source>, vlk::fragment_uniform<vk::Sampler>,
            vlk::fragment_uniform<scene::texture_transform>, vlk::fragment_uniform<vk::ImageView[6]>>(
            gpu_.dev(), gpu_.limits(), std::span<uniform_objects, 1>{&uniforms_, 1}
        )},
        pipelines_{
            gpu_.dev(), *render_pass_, gpu_.find_max_usable_samples(), descriptor_bindings_.layout(),
            vlk::shaders<scene::vertex>{
                .vertex = {_binary_triangle_vert_spv_start, _binary_triangle_vert_spv_end},
                .fragment = {_binary_triangle_frag_spv_start, _binary_triangle_frag_spv_end}
            }
        },
        mesh_{mempools_, gpu_.dev(), cmd_buffs_.queue(), cmd_buffs_.front(), scene::make_paper()},
        render_finished_{gpu_.dev(), vk::SemaphoreCreateInfo{}},
        image_available_{gpu_.dev(), vk::SemaphoreCreateInfo{}},
        frame_done_{gpu_.dev(), vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled)} {
    uniforms_.world->camera = scene::setup_camera(swapchain_info.imageExtent);
    uniforms_.transformations->models[0] =
        glm::translate(glm::scale(glm::mat4{1.}, {1. / 6., 1. / 6., 1. / 6.}), {0, -12, 0});
    uniforms_.transformations->models[5] = glm::translate(glm::mat4{1.}, {-1, -2, 0});
    *uniforms_.light = {.pos = {2., 5., 15.}, .intense = 0.8, .ambient = 0.4, .attenuation = 0.01};
  }

  render_environment(const render_environment&) = delete;
  render_environment& operator=(const render_environment&) = delete;

  render_environment(render_environment&&) noexcept = default;
  render_environment& operator=(render_environment&&) noexcept = default;

  ~render_environment() noexcept {
    if (*gpu_.dev() != nullptr)
      gpu_.dev().waitIdle();
  }

  void resize(size sz) override {
    gpu_.dev().waitIdle();
    const auto extent = as_extent(sz);

    render_target_.resize(
        gpu_.dev(), gpu_.memory_properties(), *render_pass_, gpu_.find_max_usable_samples(),
        gpu_.make_swapchain_info(render_target_.surface(), render_target_.image_format(), extent), extent
    );

    if (sz.height > 0 && sz.height > 0)
      uniforms_.world->camera = scene::setup_camera(extent);
  }

  void draw(frames_clock::time_point ts) override {
    wait_last_frame_done();
    draw(render_target_.start_frame(*image_available_), ts).present(*render_finished_);
  }

private:
  void wait_last_frame_done() {
    if (auto ec = make_error_code(
            gpu_.dev().waitForFences({*frame_done_}, true, std::numeric_limits<uint64_t>::max())
        ))
      throw std::system_error(ec, "vkWaitForFence");
    gpu_.dev().resetFences({*frame_done_});
  }

  vlk::render_target::frame draw(vlk::render_target::frame frame, frames_clock::time_point ts) {
    const size_t next_uniform = (ts.time_since_epoch() / 3s) % uniforms_.castle_textures.size();
    if (std::exchange(cur_uniform_, next_uniform) != next_uniform) {
      uniforms_.switch_castle_image(gpu_.dev(), cur_uniform_);
      descriptor_bindings_.update(0, *gpu_.dev(), uniforms_);
    }
    scene::update_world(ts, *uniforms_.world);

    std::ranges::copy(
        scene::catapult{{-7, -5}}
            .turn_arm(std::sin(M_2_PI * float_time::seconds(ts.time_since_epoch()) / 1s))
            .move(std::sin(float_time::seconds(ts.time_since_epoch()) / 700ms))
            .sprites_transformations(),
        std::next(std::begin(uniforms_.transformations->models))
    );

    submit_cmd_buf(record_cmd_buffer(cmd_buffs_.front(), frame.buffer()));
    return frame;
  }

  vk::CommandBuffer record_cmd_buffer(vk::CommandBuffer cmd, const vk::Framebuffer& fb) const {
    cmd.reset();

    cmd.begin(vk::CommandBufferBeginInfo{});
    cmd.beginRenderPass(
        vk::RenderPassBeginInfo{}
            .setRenderPass(*render_pass_)
            .setFramebuffer(fb)
            .setRenderArea(vk::Rect2D{{0, 0}, render_target_.extent()})
            .setClearValues(clear_color),
        vk::SubpassContents::eInline
    );
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines_[0]);

    cmd.setViewport(
        0, {vk::Viewport{}
                .setWidth(static_cast<float>(render_target_.extent().width))
                .setHeight(static_cast<float>(render_target_.extent().height))
                .setMinDepth(0)
                .setMaxDepth(1)}
    );
    cmd.setScissor(0, {vk::Rect2D{{0, 0}, render_target_.extent()}});

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
    uniform_pools_.flush(descriptor_bindings_.flush_region(0));

    const vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    cmd_buffs_.queue().submit(
        vk::SubmitInfo{}
            .setWaitSemaphores(*image_available_)
            .setPWaitDstStageMask(&wait_stage)
            .setCommandBuffers(cmd)
            .setSignalSemaphores(*render_finished_),
        *frame_done_
    );
  }

private:
  gpu_device gpu_;

  vlk::uniform_pools uniform_pools_;
  vlk::memory_pools mempools_;

  vk::raii::RenderPass render_pass_;
  vlk::render_target render_target_;

  vlk::command_buffers<1> cmd_buffs_;
  uniform_objects uniforms_;
  vlk::pipeline_bindings<
      1, vlk::graphics_uniform<scene::world_transformations>, vlk::fragment_uniform<scene::light_source>,
      vlk::fragment_uniform<vk::Sampler>, vlk::fragment_uniform<scene::texture_transform>,
      vlk::fragment_uniform<vk::ImageView[6]>>
      descriptor_bindings_;
  vlk::pipelines_storage<1> pipelines_;
  mesh mesh_;
  size_t cur_uniform_ = 0;

  vk::raii::Semaphore render_finished_;
  vk::raii::Semaphore image_available_;
  vk::raii::Fence frame_done_;
};

auto select_suitable_device(vk::raii::Instance inst, vk::SurfaceKHR surf, vk::Extent2D sz) {
  for (vk::raii::PhysicalDevice dev : inst.enumeratePhysicalDevices()) {
    if (check_device(dev) && check_suitable_presentation_params(dev, surf)) {
      if (const auto families = device_queue_families::find(dev, surf)) {
        spdlog::info("Using Vulkan device: {}", std::string_view{dev.getProperties().deviceName});
        return gpu_device{std::move(inst), std::move(dev), *families};
      }
    }
    spdlog::debug("Vulkan device '{}' is rejected", std::string_view{dev.getProperties().deviceName});
  }

  throw std::runtime_error{"No suitable Vulkan device found"};
}

} // namespace

std::unique_ptr<renderer_iface> make_vk_renderer(wl_display& display, wl_surface& surf, size sz) {
  vk::raii::Instance inst = create_instance();
  vk::raii::SurfaceKHR vk_surf{
      inst, vk::WaylandSurfaceCreateInfoKHR{}.setDisplay(&display).setSurface(&surf)
  };

  gpu_device gpu = select_suitable_device(std::move(inst), *vk_surf, as_extent(sz));
  const auto families = gpu.families();
  const auto swapchain_info =
      gpu.make_swapchain_info(*vk_surf, *gpu.find_compatible_format_for(*vk_surf), as_extent(sz));

  return std::make_unique<render_environment>(std::move(gpu), std::move(vk_surf), families, swapchain_info);
}
