#pragma once

#include <array>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace vlk {

template <size_t N>
class command_buffers : private vk::raii::CommandPool {
public:
  command_buffers() noexcept = default;
  command_buffers(const vk::raii::Device& dev, uint32_t queue_family)
      : vk::raii::CommandPool{dev,
            vk::CommandPoolCreateInfo{
                .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = queue_family}},
        queue_{dev.getQueue(queue_family, 0)} {
    vk::CommandBufferAllocateInfo alloc_info{.commandPool = *(*this),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = N};
    // TODO: std::start_lifetime_as instead of reinterpret_casting
    dev.getDispatcher()->vkAllocateCommandBuffers(static_cast<VkDevice>(*dev),
        &static_cast<VkCommandBufferAllocateInfo&>(alloc_info),
        reinterpret_cast<VkCommandBuffer*>(bufs_.data()));
  }

  command_buffers(const command_buffers&) = delete;
  command_buffers& operator=(const command_buffers&) = delete;

  command_buffers(command_buffers&& rhs) noexcept
      : vk::raii::CommandPool{static_cast<vk::raii::CommandPool&&>(rhs)},
        queue_{std::move(rhs.queue_)}, bufs_{rhs.bufs_} {}
  command_buffers& operator=(command_buffers&& rhs) noexcept {
    clear();
    static_cast<vk::raii::CommandPool&>(*this) =
        static_cast<vk::raii::CommandPool&&>(rhs);
    queue_ = std::move(rhs.queue_);
    bufs_ = rhs.bufs_;
    return *this;
  }

  ~command_buffers() noexcept { clear(); }

  vk::Queue queue() const noexcept { return *queue_; }

  void clear() noexcept {
    if (*static_cast<vk::raii::CommandPool&>(*this))
      getDevice().freeCommandBuffers(*(*this), bufs_.size(), bufs_.data());
    vk::raii::CommandPool::clear();
  }

  constexpr size_t size() const noexcept { return bufs_.size(); }
  const vk::CommandBuffer* data() const noexcept { return bufs_.data(); }

  vk::CommandBuffer operator[](size_t idx) const noexcept { return bufs_[idx]; }
  vk::CommandBuffer front() const noexcept { return bufs_.front(); }
  vk::CommandBuffer back() const noexcept { return bufs_.back(); }

private:
  vk::raii::Queue queue_{nullptr};
  std::array<vk::CommandBuffer, N> bufs_ = {};
};

} // namespace vlk
