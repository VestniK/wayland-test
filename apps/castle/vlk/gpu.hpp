#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "cmds.hpp"
#include "vma.hpp"

namespace vlk {

struct device_queue_families {
  uint32_t graphics;
  uint32_t presentation;

  static std::optional<device_queue_families> find(vk::PhysicalDevice dev, vk::SurfaceKHR surf);
};

class gpu {
public:
  gpu() noexcept = default;
  gpu(vk::raii::Instance&& inst, vk::raii::PhysicalDevice&& dev, device_queue_families families);

  const vk::raii::Device& dev() const noexcept { return device_; }
  const vma_allocator& allocator() const noexcept { return alloc_; }

  vk::raii::Queue create_graphics_queue() const { return device_.getQueue(families_.graphics, 0); }
  vk::raii::Queue create_presentation_queue() const { return device_.getQueue(families_.presentation, 0); }

  vk::PhysicalDeviceMemoryProperties memory_properties() const noexcept {
    return phydev_.getMemoryProperties();
  }
  vk::PhysicalDeviceLimits limits() const noexcept { return phydev_.getProperties().limits; }

  vk::SampleCountFlagBits find_max_usable_samples() const noexcept;

  std::optional<vk::Format> find_compatible_format_for(vk::SurfaceKHR surf) const;

  vk::SwapchainCreateInfoKHR
  make_swapchain_info(vk::SurfaceKHR surf, vk::Format img_fmt, vk::Extent2D sz) const;

  template <size_t N>
  vlk::command_buffers<N> create_cmd_buffs() {
    return {device_, families_.graphics};
  }

private:
  vk::raii::Instance instance_{nullptr};
  vk::raii::PhysicalDevice phydev_{nullptr};
  vk::raii::Device device_{nullptr};
  device_queue_families families_{};
  vma_allocator alloc_;
};

gpu select_suitable_device(vk::raii::Instance inst, vk::SurfaceKHR surf, vk::Extent2D sz);

} // namespace vlk
