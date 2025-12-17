#include "resource_uploader.hpp"

#include <vulkan/vulkan.hpp>

namespace vlk {

resource_uploader::resource_uploader(
    const vk::raii::Device& device, const vma_allocator& allocator, vk::Queue transfer_queue,
    vk::raii::CommandPool command_pool
)
    : device_(device), allocator_(allocator), transfer_queue_(transfer_queue),
      command_pool_(std::move(command_pool)) {}

resource_uploader::staging_buffer resource_uploader::create_staging_buffer(size_t size) {
  return staging_buffer(allocator_.allocate_staging_buffer(size));
}

resource_uploader::upload_result<vlk::allocated_resource<vk::Buffer>>
resource_uploader::create_buffer_with_staging(
    vk::BufferUsageFlags usage, const data_provider& data_provider
) {
  // Create staging buffer
  auto staging = create_staging_buffer(data_provider.data_size());

  // Write data to staging buffer
  data_provider.write_data(staging.mapped_memory());
  staging.flush();

  // Create destination buffer
  auto destination =
      allocator_.allocate_buffer(vk::BufferUsageFlagBits::eTransferDst | usage, data_provider.data_size());

  // Create command buffer and fence for transfer
  vk::raii::CommandBuffer cmd_buffer = std::move(device_
                                                     .allocateCommandBuffers(
                                                         vk::CommandBufferAllocateInfo{}
                                                             .setCommandPool(*command_pool_)
                                                             .setLevel(vk::CommandBufferLevel::ePrimary)
                                                             .setCommandBufferCount(1)
                                                     )
                                                     .front());

  vk::raii::Fence transfer_fence = vk::raii::Fence(device_, vk::FenceCreateInfo{});

  // Record transfer commands
  copy_buffer_to_buffer(
      staging.underlying_buffer().resource(), destination.resource(), data_provider.data_size(), cmd_buffer
  );

  // Submit transfer command
  vk::SubmitInfo submit_info{};
  submit_info.setCommandBuffers(*cmd_buffer);
  transfer_queue_.submit(submit_info, *transfer_fence);

  return upload_result<vlk::allocated_resource<vk::Buffer>>{
      std::move(destination), std::move(transfer_fence)
  };
}

resource_uploader::upload_result<vlk::allocated_resource<vk::Buffer>>
resource_uploader::create_buffer_with_staging(vk::BufferUsageFlags usage, std::span<const std::byte> data) {
  class simple_data_provider : public data_provider {
  public:
    simple_data_provider(std::span<const std::byte> data) : data_(data) {}
    size_t data_size() const override { return data_.size(); }
    void write_data(std::span<std::byte> destination) const override {
      std::ranges::copy(data_, destination.begin());
    }

  private:
    std::span<const std::byte> data_;
  };

  simple_data_provider provider(data);
  return create_buffer_with_staging(usage, provider);
}

resource_uploader::upload_result<vlk::allocated_resource<vk::Image>>
resource_uploader::create_image_with_staging(
    vk::Format format, vk::Extent2D size, const data_provider& data_provider
) {
  // Create staging buffer
  auto staging = create_staging_buffer(data_provider.data_size());

  // Write data to staging buffer
  data_provider.write_data(staging.mapped_memory());
  staging.flush();

  // Create destination image
  auto destination = allocator_.allocate_image(format, size);

  // Create command buffer and fence for transfer
  vk::raii::CommandBuffer cmd_buffer = std::move(device_
                                                     .allocateCommandBuffers(
                                                         vk::CommandBufferAllocateInfo{}
                                                             .setCommandPool(*command_pool_)
                                                             .setLevel(vk::CommandBufferLevel::ePrimary)
                                                             .setCommandBufferCount(1)
                                                     )
                                                     .front());

  vk::raii::Fence transfer_fence = vk::raii::Fence(device_, vk::FenceCreateInfo{});

  // Record transfer commands
  copy_buffer_to_image(staging.underlying_buffer().resource(), destination.resource(), size, cmd_buffer);

  // Submit transfer command
  vk::SubmitInfo submit_info{};
  submit_info.setCommandBuffers(*cmd_buffer);
  transfer_queue_.submit(submit_info, *transfer_fence);

  return upload_result<vlk::allocated_resource<vk::Image>>{
      std::move(destination), std::move(transfer_fence)
  };
}

void resource_uploader::copy_buffer_to_buffer(
    vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size,
    const vk::raii::CommandBuffer& cmd_buffer
) {
  cmd_buffer.begin(vk::CommandBufferBeginInfo{});
  cmd_buffer.copyBuffer(src_buffer, dst_buffer, vk::BufferCopy{}.setSize(size));
  cmd_buffer.end();
}

void resource_uploader::copy_buffer_to_image(
    vk::Buffer src_buffer, vk::Image dst_image, vk::Extent2D size, const vk::raii::CommandBuffer& cmd_buffer
) {
  cmd_buffer.begin(vk::CommandBufferBeginInfo{});

  const auto img_dst_barrier = vk::ImageMemoryBarrier{}
                                   .setSrcAccessMask(vk::AccessFlagBits::eNone)
                                   .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
                                   .setOldLayout(vk::ImageLayout::eUndefined)
                                   .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                                   .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                   .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                   .setImage(dst_image)
                                   .setSubresourceRange(
                                       vk::ImageSubresourceRange{}
                                           .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                           .setLevelCount(1)
                                           .setLayerCount(1)
                                   );
  cmd_buffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, img_dst_barrier
  );

  auto copy_region =
      vk::BufferImageCopy{}
          .setImageExtent(vk::Extent3D{size, 1})
          .setImageSubresource(
              vk::ImageSubresourceLayers{}.setLayerCount(1).setAspectMask(vk::ImageAspectFlagBits::eColor)
          );
  cmd_buffer.copyBufferToImage(src_buffer, dst_image, vk::ImageLayout::eTransferDstOptimal, copy_region);

  const auto img_sampler_barrier = vk::ImageMemoryBarrier{}
                                       .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                                       .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                                       .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                                       .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                                       .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                       .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                       .setImage(dst_image)
                                       .setSubresourceRange(
                                           vk::ImageSubresourceRange{}
                                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                               .setLevelCount(1)
                                               .setLayerCount(1)
                                       );
  cmd_buffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
      img_sampler_barrier
  );

  cmd_buffer.end();
}

void resource_uploader::wait_for_transfer_completion(const vk::raii::Fence& fence) {
  if (auto ec = make_error_code(device_.waitForFences({*fence}, true, std::numeric_limits<uint64_t>::max())))
    throw std::system_error(ec, "vkWaitForFence");
  device_.resetFences({*fence});
}

} // namespace vlk