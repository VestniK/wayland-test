#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace vlk {

class swapchain {
public:
  class frame;

  class available_framebuffer {
  public:
    available_framebuffer() noexcept = default;
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
  swapchain() noexcept;
  swapchain(const vk::raii::Device& device, const vk::SurfaceKHR& surf,
      const vk::SwapchainCreateInfoKHR& swapchain_info,
      const vk::RenderPass& render_pass);
  ~swapchain() noexcept;

  swapchain(swapchain&&) noexcept;
  swapchain& operator=(swapchain&&) noexcept;

  swapchain(const swapchain&) noexcept = delete;
  swapchain& operator=(const swapchain&) noexcept = delete;

  void clear() noexcept;

  vk::Format image_format() const noexcept { return swapchain_image_format_; }
  vk::Extent2D extent() const noexcept { return swapchain_extent_; }

  available_framebuffer acqure_framebuffer(
      const vk::Semaphore& image_available) const;

  void present(available_framebuffer&& fb, const vk::Queue& presentation_queue,
      const vk::Semaphore& done_sem) const {
    fb.present(*swapchain_, presentation_queue, done_sem);
  }

private:
  vk::raii::SwapchainKHR swapchain_{nullptr};
  std::vector<frame> frames_;
  vk::Format swapchain_image_format_;
  vk::Extent2D swapchain_extent_;
};

class render_target {
public:
  render_target() noexcept = default;

  render_target(const vk::raii::Device& dev, uint32_t presentation_queue_family,
      vk::raii::SurfaceKHR surf,
      const vk::SwapchainCreateInfoKHR& swapchain_info,
      const vk::RenderPass& render_pass)
      : presentation_queue_{dev.getQueue(presentation_queue_family, 0)},
        surf_{std::move(surf)},
        swapchain_{dev, *surf_, swapchain_info, render_pass} {}

  void resize(const vk::raii::Device& dev, vk::RenderPass render_pass,
      const vk::SwapchainCreateInfoKHR& swapchain_info, vk::Extent2D sz);

  vk::SurfaceKHR surface() const noexcept { return *surf_; }

  vk::Extent2D extent() const noexcept { return swapchain_.extent(); }

  class frame {
  public:
    frame() noexcept = default;
    frame(const render_target* parent, vk::Semaphore ready_to_present)
        : parent_{parent},
          fb_{parent->swapchain_.acqure_framebuffer(ready_to_present)} {}

    vk::Framebuffer buffer() const { return fb_.framebuffer(); }

    void present(vk::Semaphore done_sem) && {
      parent_->swapchain_.present(
          std::move(fb_), *parent_->presentation_queue_, done_sem);
    }

  private:
    const render_target* parent_ = nullptr;
    swapchain::available_framebuffer fb_;
  };

  frame start_frame(vk::Semaphore ready_to_present) const {
    return {this, ready_to_present};
  }

private:
  vk::raii::Queue presentation_queue_{nullptr};
  vk::raii::SurfaceKHR surf_{nullptr};
  swapchain swapchain_;
};

} // namespace vlk
