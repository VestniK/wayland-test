module;

#include <limits>
#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

export module vlk:presentation;

import :buf;

namespace vlk {

export class swapchain {
public:
  class frame;

  class available_framebuffer {
  public:
    available_framebuffer() noexcept = default;
    available_framebuffer(vk::Framebuffer fb, vk::Semaphore done_sem, uint32_t idx) noexcept
        : fb_{std::move(fb)}, done_sem_{done_sem}, idx_{idx} {}

    available_framebuffer(const available_framebuffer&) = delete;
    available_framebuffer& operator=(const available_framebuffer&) = delete;

    available_framebuffer(available_framebuffer&&) noexcept = default;
    available_framebuffer& operator=(available_framebuffer&&) noexcept = default;

    ~available_framebuffer() noexcept = default;

    const vk::Framebuffer& framebuffer() const noexcept { return fb_; }
    const vk::Semaphore& render_done_sem() const noexcept { return done_sem_; }

    void present(const vk::SwapchainKHR& swpchain, const vk::Queue& presentation_queue) const {
      const auto ec = make_error_code(presentation_queue.presentKHR(
          vk::PresentInfoKHR{}.setWaitSemaphores(done_sem_).setSwapchains(swpchain).setPImageIndices(&idx_)
      ));
      if (ec)
        throw std::system_error(ec, "vkQueuePresentKHR");
    }

  private:
    vk::Framebuffer fb_;
    vk::Semaphore done_sem_;
    uint32_t idx_;
  };

public:
  swapchain() noexcept;
  swapchain(
      const vk::raii::Device& device, const vk::PhysicalDeviceMemoryProperties& props,
      const vk::SurfaceKHR& surf, const vk::SwapchainCreateInfoKHR& swapchain_info,
      const vk::RenderPass& render_pass, vk::SampleCountFlagBits samples
  );
  ~swapchain() noexcept;

  swapchain(swapchain&&) noexcept;
  swapchain& operator=(swapchain&&) noexcept;

  swapchain(const swapchain&) noexcept = delete;
  swapchain& operator=(const swapchain&) noexcept = delete;

  void clear() noexcept;

  vk::Format image_format() const noexcept { return swapchain_image_format_; }
  vk::Extent2D extent() const noexcept { return swapchain_extent_; }

  available_framebuffer acqure_framebuffer(const vk::Semaphore& image_available) const;

  void present(available_framebuffer&& fb, const vk::Queue& presentation_queue) const {
    fb.present(*swapchain_, presentation_queue);
  }

private:
  vk::raii::SwapchainKHR swapchain_{nullptr};
  vk::raii::Image multisampling_img_{nullptr};
  vlk::memory multisampling_mem_;
  vk::raii::ImageView multisampling_view_{nullptr};
  std::vector<frame> frames_;
  vk::Format swapchain_image_format_;
  vk::Extent2D swapchain_extent_;
};

export class render_target {
public:
  render_target() noexcept = default;

  render_target(
      const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props,
      vk::raii::Queue presentation_queue, vk::raii::SurfaceKHR surf,
      const vk::SwapchainCreateInfoKHR& swapchain_info, const vk::RenderPass& render_pass,
      vk::SampleCountFlagBits samples
  )
      : presentation_queue_{std::move(presentation_queue)}, surf_{std::move(surf)},
        swapchain_{dev, props, *surf_, swapchain_info, render_pass, samples} {}

  void resize(
      const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props,
      vk::RenderPass render_pass, vk::SampleCountFlagBits samples,
      const vk::SwapchainCreateInfoKHR& swapchain_info, vk::Extent2D sz
  );

  vk::SurfaceKHR surface() const noexcept { return *surf_; }

  vk::Extent2D extent() const noexcept { return swapchain_.extent(); }

  vk::Format image_format() const noexcept { return swapchain_.image_format(); }

  class frame {
  public:
    frame() noexcept = default;
    frame(const render_target* parent, vk::Semaphore ready_to_present)
        : parent_{parent}, fb_{parent->swapchain_.acqure_framebuffer(ready_to_present)} {}

    vk::Framebuffer buffer() const { return fb_.framebuffer(); }
    vk::Semaphore render_done_sem() { return fb_.render_done_sem(); }

    void present() && { parent_->swapchain_.present(std::move(fb_), *parent_->presentation_queue_); }

  private:
    const render_target* parent_ = nullptr;
    swapchain::available_framebuffer fb_;
  };

  frame start_frame(vk::Semaphore ready_to_present) const { return {this, ready_to_present}; }

private:
  vk::raii::Queue presentation_queue_{nullptr};
  vk::raii::SurfaceKHR surf_{nullptr};
  swapchain swapchain_;
};

} // namespace vlk

namespace vlk {

namespace {

vk::raii::Framebuffer make_fb_helper(
    const vk::raii::Device& dev, vk::RenderPass pass, vk::Extent2D sz,
    std::array<vk::ImageView, 2> attachements
) {
  return vk::raii::Framebuffer{
      dev, vk::FramebufferCreateInfo{}
               .setRenderPass(pass)
               .setAttachments(attachements)
               .setWidth(sz.width)
               .setHeight(sz.height)
               .setLayers(1)
  };
}

} // namespace

class swapchain::frame {
public:
  constexpr frame() noexcept = default;

  frame(
      const vk::raii::Device& dev, vk::RenderPass render_pass, vk::ImageView multisampling_img,
      vk::Image image, vk::Extent2D extent, vk::Format fmt
  )
      : view_{
            dev, vk::ImageViewCreateInfo{}
                     .setImage(image)
                     .setViewType(vk::ImageViewType::e2D)
                     .setFormat(fmt)
                     .setSubresourceRange(
                         vk::ImageSubresourceRange{}
                             .setAspectMask(vk::ImageAspectFlagBits::eColor)
                             .setLevelCount(1)
                             .setLayerCount(1)
                     )
        },
        framebuf_{make_fb_helper(dev, render_pass, extent, {multisampling_img, *view_})},
        render_finished_{dev, vk::SemaphoreCreateInfo{}} {}

  static std::vector<frame> from_images(
      const vk::raii::Device& dev, vk::RenderPass render_pass, vk::ImageView multisampling_img,
      std::span<const vk::Image> images, vk::Extent2D extent, vk::Format fmt
  ) {
    std::vector<frame> res;
    res.reserve(images.size());
    for (vk::Image img : images)
      res.push_back({dev, render_pass, multisampling_img, img, extent, fmt});
    return res;
  }

  vk::ImageView image_view() const noexcept { return *view_; }
  vk::Framebuffer frameuffer() const noexcept { return *framebuf_; }
  vk::Semaphore render_done_sem() const noexcept { return *render_finished_; }

private:
  vk::raii::ImageView view_{nullptr};
  vk::raii::Framebuffer framebuf_{nullptr};
  vk::raii::Semaphore render_finished_{nullptr};
};

swapchain::swapchain() noexcept = default;

swapchain::swapchain(
    const vk::raii::Device& device, const vk::PhysicalDeviceMemoryProperties& props,
    const vk::SurfaceKHR& surf, const vk::SwapchainCreateInfoKHR& swapchain_info,
    const vk::RenderPass& render_pass, vk::SampleCountFlagBits samples
)
    : swapchain_{device, swapchain_info},
      multisampling_img_{device.createImage(
          vk::ImageCreateInfo{}
              .setImageType(vk::ImageType::e2D)
              .setFormat(swapchain_info.imageFormat)
              .setExtent(vk::Extent3D{swapchain_info.imageExtent, 1})
              .setMipLevels(1)
              .setArrayLayers(1)
              .setSamples(samples)
              .setUsage(
                  vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment
              )
      )},
      swapchain_image_format_{swapchain_info.imageFormat}, swapchain_extent_{swapchain_info.imageExtent} {
  const auto req = multisampling_img_.getMemoryRequirements();
  multisampling_mem_ = vlk::memory::alocate(device, props, req.memoryTypeBits, req.size);
  (*device).bindImageMemory(*multisampling_img_, *multisampling_mem_.get(), 0);
  multisampling_view_ = device.createImageView(
      vk::ImageViewCreateInfo{}
          .setImage(multisampling_img_)
          .setViewType(vk::ImageViewType::e2D)
          .setFormat(swapchain_image_format_)
          .setSubresourceRange(
              vk::ImageSubresourceRange{}
                  .setAspectMask(vk::ImageAspectFlagBits::eColor)
                  .setLevelCount(1)
                  .setLayerCount(1)
          )
  );
  frames_ = frame::from_images(
      device, render_pass, *multisampling_view_, swapchain_.getImages(), swapchain_info.imageExtent,
      swapchain_info.imageFormat
  );
}

swapchain::~swapchain() noexcept = default;

swapchain::swapchain(swapchain&&) noexcept = default;
swapchain& swapchain::operator=(swapchain&&) noexcept = default;

void swapchain::clear() noexcept {
  frames_.clear();
  swapchain_.clear();
}

swapchain::available_framebuffer swapchain::acqure_framebuffer(const vk::Semaphore& image_available) const {
  const auto [res, idx] = swapchain_.acquireNextImage(std::numeric_limits<uint32_t>::max(), image_available);
  auto ec = make_error_code(res);
  if (ec)
    throw std::system_error(ec, "vkAcquireNextImageKHR");
  assert(idx < frames_.size());
  return {frames_[idx].frameuffer(), frames_[idx].render_done_sem(), idx};
}

void render_target::resize(
    const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props, vk::RenderPass render_pass,
    vk::SampleCountFlagBits samples, const vk::SwapchainCreateInfoKHR& swapchain_info, vk::Extent2D sz
) {
  // render_pass is keept untoched but was created with old image format
  // choosin new format might be not safe
  assert(swapchain_info.imageFormat == swapchain_.image_format());
  swapchain_.clear();
  swapchain_ = swapchain{dev, props, *surf_, swapchain_info, render_pass, samples};
}

} // namespace vlk
