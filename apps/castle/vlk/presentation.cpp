#include "presentation.hpp"

#include <limits>
#include <span>

namespace vlk {

namespace {

vk::raii::Framebuffer make_fb_helper(const vk::raii::Device& dev,
    vk::RenderPass pass, vk::Extent2D sz,
    std::array<vk::ImageView, 2> attachements) {
  return vk::raii::Framebuffer{
      dev, vk::FramebufferCreateInfo{.renderPass = pass,
               .attachmentCount = attachements.size(),
               .pAttachments = attachements.data(),
               .width = sz.width,
               .height = sz.height,
               .layers = 1}};
}

} // namespace

class swapchain::frame {
public:
  constexpr frame() noexcept = default;

  frame(const vk::raii::Device& dev, vk::RenderPass render_pass,
      vk::ImageView multisampling_img, vk::Image image, vk::Extent2D extent,
      vk::Format fmt)
      : view_{dev, vk::ImageViewCreateInfo{.image = image,
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
        framebuf_{make_fb_helper(
            dev, render_pass, extent, {multisampling_img, *view_})} {}

  static std::vector<frame> from_images(const vk::raii::Device& dev,
      vk::RenderPass render_pass, vk::ImageView multisampling_img,
      std::span<const vk::Image> images, vk::Extent2D extent, vk::Format fmt) {
    std::vector<frame> res;
    res.reserve(images.size());
    for (vk::Image img : images)
      res.push_back({dev, render_pass, multisampling_img, img, extent, fmt});
    return res;
  }

  vk::ImageView image_view() const noexcept { return *view_; }
  vk::Framebuffer frameuffer() const noexcept { return *framebuf_; }

private:
  vk::raii::ImageView view_{nullptr};
  vk::raii::Framebuffer framebuf_{nullptr};
};

swapchain::swapchain() noexcept = default;

swapchain::swapchain(const vk::raii::Device& device,
    const vk::PhysicalDeviceMemoryProperties& props, const vk::SurfaceKHR& surf,
    const vk::SwapchainCreateInfoKHR& swapchain_info,
    const vk::RenderPass& render_pass, vk::SampleCountFlagBits samples)
    : swapchain_{device, swapchain_info},
      multisampling_img_{device.createImage(vk::ImageCreateInfo{.flags = {},
          .imageType = vk::ImageType::e2D,
          .format = swapchain_info.imageFormat,
          .extent = {.width = swapchain_info.imageExtent.width,
              .height = swapchain_info.imageExtent.height,
              .depth = 1},
          .mipLevels = 1,
          .arrayLayers = 1,
          .samples = samples,
          .tiling = vk::ImageTiling::eOptimal,
          .usage = vk::ImageUsageFlagBits::eTransientAttachment |
                   vk::ImageUsageFlagBits::eColorAttachment,
          .sharingMode = vk::SharingMode::eExclusive,
          .queueFamilyIndexCount = {},
          .pQueueFamilyIndices = {},
          .initialLayout = vk::ImageLayout::eUndefined})},
      swapchain_image_format_{swapchain_info.imageFormat},
      swapchain_extent_{swapchain_info.imageExtent} {
  const auto req = multisampling_img_.getMemoryRequirements();
  multisampling_mem_ =
      vlk::memory::alocate(device, props, req.memoryTypeBits, req.size);
  (*device).bindImageMemory(*multisampling_img_, *multisampling_mem_.get(), 0);
  multisampling_view_ =
      device.createImageView(vk::ImageViewCreateInfo{.flags = {},
          .image = *multisampling_img_,
          .viewType = vk::ImageViewType::e2D,
          .format = swapchain_image_format_,
          .components = {},
          .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1}});
  frames_ = frame::from_images(device, render_pass, *multisampling_view_,
      swapchain_.getImages(), swapchain_info.imageExtent,
      swapchain_info.imageFormat);
}

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

void render_target::resize(const vk::raii::Device& dev,
    const vk::PhysicalDeviceMemoryProperties& props, vk::RenderPass render_pass,
    vk::SampleCountFlagBits samples,
    const vk::SwapchainCreateInfoKHR& swapchain_info, vk::Extent2D sz) {
  // render_pass is keept untoched but was created with old image format
  // choosin new format might be not safe
  assert(swapchain_info.imageFormat == swapchain_.image_format());
  swapchain_.clear();
  swapchain_ =
      swapchain{dev, props, *surf_, swapchain_info, render_pass, samples};
}

} // namespace vlk
