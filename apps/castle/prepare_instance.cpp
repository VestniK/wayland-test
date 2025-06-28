#include "prepare_instance.hpp"

#include <algorithm>
#include <optional>
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
#include <libs/sfx/sfx.hpp>

#include <apps/castle/vlk/buf.hpp>
#include <apps/castle/vlk/cmds.hpp>
#include <apps/castle/vlk/gpu.hpp>
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

class mesh {
public:
  mesh(
      const vlk::vma_allocator& alloc, vk::Queue transfer_queue, vk::CommandBuffer copy_cmd,
      vlk::mesh_data_view<scene::vertex, uint16_t> data
  )
      : vbo_{load_buffer(
            alloc, transfer_queue, copy_cmd, vk::BufferUsageFlagBits::eVertexBuffer,
            std::as_bytes(data.vertices)
        )},
        ibo_{load_buffer(
            alloc, transfer_queue, copy_cmd, vk::BufferUsageFlagBits::eIndexBuffer,
            std::as_bytes(data.indices)
        )},
        count_{data.indices.size()} {}

  vk::Buffer get_vbo() const noexcept { return vbo_.resource(); }
  vk::Buffer get_ibo() const noexcept { return ibo_.resource(); }
  const size_t size() const noexcept { return count_; }

private:
  static vlk::allocated_resource<VkBuffer> load_buffer(
      const vlk::vma_allocator& alloc, vk::Queue transfer_queue, vk::CommandBuffer cmd,
      vk::BufferUsageFlagBits usage, std::span<const std::byte> data
  ) {
    auto staging = alloc.allocate_staging_buffer(data.size());
    std::ranges::copy(data, staging.mapping().data());
    auto res = alloc.allocate_buffer(vk::BufferUsageFlagBits::eTransferDst | usage, data.size());
    staging.flush();
    vlk::copy(transfer_queue, cmd, staging.resource(), res.resource(), data.size());
    return res;
  }

private:
  vlk::allocated_resource<VkBuffer> vbo_;
  vlk::allocated_resource<VkBuffer> ibo_;
  size_t count_;
};

constexpr vk::Format to_vk_fmt(img::pixel_fmt fmt) noexcept {
  switch (fmt) {
  case img::pixel_fmt::rgb:
    return vk::Format::eR8G8B8Srgb;
  case img::pixel_fmt::rgba:
    return vk::Format::eR8G8B8A8Srgb;
  case img::pixel_fmt::grayscale:
    return vk::Format::eR8Srgb;
  }
  std::unreachable();
}

vlk::allocated_resource<VkImage> create_texture(
    img::reader& reader, const vlk::vma_allocator& alloc, vk::Queue transfer_queue, vk::CommandBuffer cmd
) {
  auto staging = alloc.allocate_staging_buffer(reader.pixels_size());
  reader.read_pixels(staging.mapping());
  staging.flush();
  auto res = alloc.allocate_image(to_vk_fmt(reader.format()), as_extent(reader.size()));
  vlk::copy(transfer_queue, cmd, staging.resource(), res.resource(), as_extent(reader.size()));
  return res;
}

vlk::allocated_resource<VkImage> load_text_texture(
    const vlk::vma_allocator& alloc, vk::Queue transfer_queue, vk::CommandBuffer cmd, std::string_view text
) {
  auto resources = sfx::archive::open_self();
  auto& fd = resources.open("fonts/RuthlessSketch.ttf");
  auto font = img::font::load(fd);
  auto reader = font.text_image_reader(text);
  return create_texture(reader, alloc, transfer_queue, cmd);
}

vlk::allocated_resource<VkImage> load_sfx_texture(
    const vlk::vma_allocator& alloc, vk::Queue transfer_queue, vk::CommandBuffer cmd, const fs::path& sfx_path
) {
  auto resources = sfx::archive::open_self();
  auto& fd = resources.open(sfx_path);
  auto reader = img::load_reader(fd);
  return create_texture(reader, alloc, transfer_queue, cmd);
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

std::tuple<vlk::allocated_resource<VkImage>, vk::raii::ImageView>
make_image_view(const vk::raii::Device& dev, vlk::allocated_resource<VkImage>&& img) {
  auto view = make_view(dev, img.resource());
  return std::tuple{std::move(img), std::move(view)};
}

struct uniform_objects {
  uniform_objects(
      const vk::raii::Device& dev, const vk::PhysicalDeviceLimits& limits,
      std::array<vlk::allocated_resource<VkImage>, 4> img, vlk::allocated_resource<VkImage> fwheel,
      vlk::allocated_resource<VkImage> rwheel, vlk::allocated_resource<VkImage> platform,
      vlk::allocated_resource<VkImage> arm, vlk::allocated_resource<VkImage> word_img
  )
      : sampler{make_sampler(dev, limits)}, castle_textures{std::move(img)},
        castle_texture_view{make_view(dev, castle_textures[0].resource())},
        catapult{
            make_image_view(dev, std::move(fwheel)), make_image_view(dev, std::move(rwheel)),
            make_image_view(dev, std::move(platform)), make_image_view(dev, std::move(arm))
        },
        word{std::move(word_img)}, word_view{make_view(dev, word.resource())} {}

  vlk::ubo::unique_ptr<scene::world_transformations> world;
  vlk::ubo::unique_ptr<scene::light_source> light;
  vlk::ubo::unique_ptr<scene::texture_transform> transformations;

  vk::raii::Sampler sampler;
  std::array<vlk::allocated_resource<VkImage>, 4> castle_textures;
  vk::raii::ImageView castle_texture_view;

  std::array<std::tuple<vlk::allocated_resource<VkImage>, vk::raii::ImageView>, 4> catapult;
  vlk::allocated_resource<VkImage> word;
  vk::raii::ImageView word_view;

  void switch_castle_image(const vk::raii::Device& dev, size_t n) {
    castle_texture_view = make_view(dev, castle_textures[n].resource());
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
  render_environment(vlk::gpu gpu, vk::raii::SurfaceKHR surf, vk::SwapchainCreateInfoKHR swapchain_info)
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
        render_pass_{make_render_pass(gpu_.dev(), swapchain_info.imageFormat, gpu_.find_max_usable_samples())
        },
        render_target_{
            gpu_.dev(),
            gpu_.memory_properties(),
            gpu_.create_presentation_queue(),
            std::move(surf),
            swapchain_info,
            *render_pass_,
            gpu_.find_max_usable_samples()
        },
        cmd_buffs_{gpu_.create_cmd_buffs<1>()},
        uniforms_{
            gpu_.dev(),
            gpu_.limits(),
            {load_sfx_texture(
                 gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/castle-0hit.png"
             ),
             load_sfx_texture(
                 gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/castle-1hit.png"
             ),
             load_sfx_texture(
                 gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/castle-2hit.png"
             ),
             load_sfx_texture(
                 gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/castle-3hit.png"
             )},
            load_sfx_texture(
                gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/catapult-front-wheel.png"
            ),
            load_sfx_texture(
                gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/catapult-rear-wheel.png"
            ),
            load_sfx_texture(
                gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/catapult-platform.png"
            ),
            load_sfx_texture(
                gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "textures/catapult-arm.png"
            ),
            load_text_texture(gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), "привет")
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
        mesh_{gpu_.allocator(), cmd_buffs_.queue(), cmd_buffs_.front(), scene::make_paper()},
        image_available_{gpu_.dev(), vk::SemaphoreCreateInfo{}},
        frame_done_{gpu_.dev(), vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled)} {
    uniforms_.world->camera = scene::setup_camera(render_target_.extent());
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
    draw(render_target_.start_frame(*image_available_), ts).present();
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

    submit_cmd_buf(record_cmd_buffer(cmd_buffs_.front(), frame.buffer()), frame.render_done_sem());
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
    const auto vbo = mesh_.get_vbo();
    cmd.bindVertexBuffers(0, 1, &vbo, offsets);
    cmd.bindIndexBuffer(mesh_.get_ibo(), 0, vk::IndexType::eUint16);

    descriptor_bindings_.use(0, cmd, pipelines_.layout());
    cmd.drawIndexed(static_cast<uint32_t>(mesh_.size()), 1, 0, 0, 0);
    cmd.endRenderPass();
    cmd.end();
    return cmd;
  }

  void submit_cmd_buf(const vk::CommandBuffer& cmd, vk::Semaphore render_done) const {
    uniform_pools_.flush(descriptor_bindings_.flush_region(0));

    const vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    cmd_buffs_.queue().submit(
        vk::SubmitInfo{}
            .setWaitSemaphores(*image_available_)
            .setPWaitDstStageMask(&wait_stage)
            .setCommandBuffers(cmd)
            .setSignalSemaphores(render_done),
        *frame_done_
    );
  }

private:
  vlk::gpu gpu_;

  vlk::uniform_pools uniform_pools_;

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

  vk::raii::Semaphore image_available_;
  vk::raii::Fence frame_done_;
};

} // namespace

std::unique_ptr<renderer_iface> make_vk_renderer(wl_display& display, wl_surface& surf, size sz) {
  vk::raii::Instance inst = create_instance();
  vk::raii::SurfaceKHR vk_surf{
      inst, vk::WaylandSurfaceCreateInfoKHR{}.setDisplay(&display).setSurface(&surf)
  };

  vlk::gpu gpu = vlk::select_suitable_device(std::move(inst), *vk_surf, as_extent(sz));
  const auto swapchain_info =
      gpu.make_swapchain_info(*vk_surf, *gpu.find_compatible_format_for(*vk_surf), as_extent(sz));

  return std::make_unique<render_environment>(std::move(gpu), std::move(vk_surf), swapchain_info);
}
