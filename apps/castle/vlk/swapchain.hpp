#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace vlk {

class swapchain {
public:
  class frame;

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
  vk::raii::SwapchainKHR swapchain_;
  std::vector<frame> frames_;
  vk::Format swapchain_image_format_;
  vk::Extent2D swapchain_extent_;
};

} // namespace vlk
