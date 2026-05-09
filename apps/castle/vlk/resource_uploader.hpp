#pragma once

#include <span>

#include <vulkan/vulkan_raii.hpp>

#include "vma.hpp"

namespace img {
class reader;
}

namespace vlk {

class resource_uploader {
public:
  resource_uploader(
      const vk::raii::Device& device, vma_allocator allocator, vk::Queue transfer_queue
  ) noexcept
      : device_{device}, allocator_{allocator}, transfer_queue_{transfer_queue} {}

  vlk::allocated_resource<vk::Buffer>
  create_buffer(vk::CommandBuffer cmd, vk::BufferUsageFlags usage, std::span<const std::byte> data);

  vlk::allocated_resource<vk::Image> create_image(vk::CommandBuffer cmd, img::reader&& reader);

private:
  void copy_buffer_to_buffer(
      vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size, vk::CommandBuffer cmd_buffer
  );

  void copy_buffer_to_image(
      vk::Buffer src_buffer, vk::Image dst_image, vk::Extent2D size, vk::CommandBuffer cmd_buffer
  );

private:
  const vk::raii::Device& device_;
  vma_allocator allocator_;
  vk::Queue transfer_queue_;
};

} // namespace vlk
