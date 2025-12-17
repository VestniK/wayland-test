#pragma once

#include <span>

#include <vulkan/vulkan_raii.hpp>

#include "buf.hpp"
#include "cmds.hpp"
#include "vma.hpp"

namespace vlk {

class resource_uploader {
public:
  // Interface for data providers that can write directly to mapped memory
  class data_provider {
  public:
    virtual ~data_provider() = default;
    virtual size_t data_size() const = 0;
    virtual void write_data(std::span<std::byte> destination) const = 0;
  };

  // Staging buffer wrapper that provides mapped memory access
  class staging_buffer {
  public:
    staging_buffer() = default;
    staging_buffer(vlk::staging_buf buffer) : buffer_(std::move(buffer)) {}

    std::span<std::byte> mapped_memory() const { return buffer_.mapping(); }

    const vlk::staging_buf& underlying_buffer() const { return buffer_; }

    void flush() { buffer_.flush(); }

  private:
    vlk::staging_buf buffer_;
  };

  // Result of a GPU resource creation operation
  template <typename ResourceType>
  struct upload_result {
    ResourceType resource;
    vk::raii::Fence transfer_fence;
  };

  resource_uploader(
      const vk::raii::Device& device, const vma_allocator& allocator, vk::Queue transfer_queue,
      vk::raii::CommandPool command_pool
  );

  // Buffer operations with staging
  upload_result<vlk::allocated_resource<vk::Buffer>>
  create_buffer_with_staging(vk::BufferUsageFlags usage, const data_provider& data_provider);

  // Convenience method for simple byte spans
  upload_result<vlk::allocated_resource<vk::Buffer>>
  create_buffer_with_staging(vk::BufferUsageFlags usage, std::span<const std::byte> data);

  // Image operations with staging
  upload_result<vlk::allocated_resource<vk::Image>>
  create_image_with_staging(vk::Format format, vk::Extent2D size, const data_provider& data_provider);

  // Direct staging buffer creation for data providers
  staging_buffer create_staging_buffer(size_t size);

  // Synchronization
  void wait_for_transfer_completion(const vk::raii::Fence& fence);

  // Accessors
  const vk::raii::Device& device() const { return device_; }
  const vma_allocator& allocator() const { return allocator_; }

private:
  const vk::raii::Device& device_;
  const vma_allocator& allocator_;
  vk::Queue transfer_queue_;
  vk::raii::CommandPool command_pool_;

  // Helper methods
  void copy_buffer_to_buffer(
      vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size,
      const vk::raii::CommandBuffer& cmd_buffer
  );

  void copy_buffer_to_image(
      vk::Buffer src_buffer, vk::Image dst_image, vk::Extent2D size, const vk::raii::CommandBuffer& cmd_buffer
  );
};

} // namespace vlk