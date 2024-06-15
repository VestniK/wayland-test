#include "swapchain.hpp"

#include <limits>
#include <span>

namespace vlk {

class swapchain::frame {
public:
  constexpr frame() noexcept = default;

  frame(const vk::raii::Device& dev, vk::RenderPass render_pass,
      vk::Image image, vk::Extent2D extent, vk::Format fmt)
      : image_{image},
        view_{dev, vk::ImageViewCreateInfo{.image = image_,
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

swapchain::swapchain(const vk::raii::Device& device, const vk::SurfaceKHR& surf,
    const vk::SwapchainCreateInfoKHR& swapchain_info,
    const vk::RenderPass& render_pass)
    : swapchain_{device, swapchain_info},
      frames_{frame::from_images(device, render_pass, swapchain_.getImages(),
          swapchain_info.imageExtent, swapchain_info.imageFormat)},
      swapchain_image_format_{swapchain_info.imageFormat},
      swapchain_extent_{swapchain_info.imageExtent} {}

swapchain::~swapchain() noexcept = default;

swapchain::swapchain(swapchain&&) noexcept = default;
swapchain& swapchain::operator=(swapchain&&) noexcept = default;

void swapchain::clear() noexcept {
  frames_.clear();
  swapchain_.clear();
}

swapchain::available_framebuffer swapchain::acqure_framebuffer(
    const vk::Semaphore& image_available) const {
  const auto [res, idx] = swapchain_.acquireNextImage(
      std::numeric_limits<uint32_t>::max(), image_available);
  auto ec = make_error_code(res);
  if (ec)
    throw std::system_error(ec, "vkAcquireNextImageKHR");
  assert(idx < frames_.size());
  return {frames_[idx].frameuffer(), idx};
}

} // namespace vlk
