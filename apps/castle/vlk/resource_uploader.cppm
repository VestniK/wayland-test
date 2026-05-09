module;

#include <limits>
#include <vulkan/vulkan.hpp>

#include <libs/img/reader.hpp>

export module vlk.resource_uploader;

import vlk.vma;

namespace vlk {

export class resource_uploader {
public:
  resource_uploader(vk::Device device, vma_allocator allocator, vk::Queue transfer_queue) noexcept
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
  vk::Device device_;
  vma_allocator allocator_;
  vk::Queue transfer_queue_;
};

namespace {

constexpr vk::Extent2D as_extent(size sz) noexcept {
  return {static_cast<uint32_t>(sz.width), static_cast<uint32_t>(sz.height)};
}

constexpr vk::Format to_vk_fmt(img::pixel_fmt fmt) noexcept {
  switch (fmt) {
  case img::pixel_fmt::rgb:
    return vk::Format::eR8G8B8Srgb;
  case img::pixel_fmt::rgba:
    return vk::Format::eR8G8B8A8Srgb;
  case img::pixel_fmt::grayscale:
    return vk::Format::eR8Srgb;
  }
  std::unreachable();
}

class temp_fence {
public:
  explicit temp_fence(vk::Device device) noexcept
      : device_{device}, fence_{device_.createFence(vk::FenceCreateInfo{})} {}

  ~temp_fence() noexcept { device_.destroyFence(fence_); }

  void wait() {
    if (auto ec =
            make_error_code(device_.waitForFences({fence_}, true, std::numeric_limits<uint64_t>::max())))
      throw std::system_error(ec, "vkWaitForFence");
  }

  vk::Fence get() const noexcept { return fence_; }

private:
  vk::Device device_;
  vk::Fence fence_;
};

} // namespace

vlk::allocated_resource<vk::Buffer> resource_uploader::create_buffer(
    vk::CommandBuffer cmd, vk::BufferUsageFlags usage, std::span<const std::byte> data
) {
  auto staging = allocator_.allocate_staging_buffer(data.size());
  std::ranges::copy(data, staging.mapping().data());
  staging.flush();

  auto destination = allocator_.allocate_buffer(vk::BufferUsageFlagBits::eTransferDst | usage, data.size());

  temp_fence transfer_fence{device_};

  // Record transfer commands
  copy_buffer_to_buffer(staging.resource(), destination.resource(), data.size(), cmd);

  // Submit transfer command
  vk::SubmitInfo submit_info{};
  submit_info.setCommandBuffers(cmd);
  transfer_queue_.submit(submit_info, transfer_fence.get());
  transfer_fence.wait();

  return destination;
};

vlk::allocated_resource<vk::Image>
resource_uploader::create_image(vk::CommandBuffer cmd, img::reader&& reader) {
  auto staging = allocator_.allocate_staging_buffer(reader.pixels_size());

  reader.read_pixels(staging.mapping());
  staging.flush();

  auto destination = allocator_.allocate_image(to_vk_fmt(reader.format()), as_extent(reader.size()));

  temp_fence transfer_fence{device_};

  copy_buffer_to_image(staging.resource(), destination.resource(), as_extent(reader.size()), cmd);

  vk::SubmitInfo submit_info{};
  submit_info.setCommandBuffers(cmd);
  transfer_queue_.submit(submit_info, transfer_fence.get());
  transfer_fence.wait();
  return destination;
}

void resource_uploader::copy_buffer_to_buffer(
    vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size, vk::CommandBuffer cmd_buffer
) {
  cmd_buffer.begin(vk::CommandBufferBeginInfo{});
  cmd_buffer.copyBuffer(src_buffer, dst_buffer, vk::BufferCopy{}.setSize(size));
  cmd_buffer.end();
}

void resource_uploader::copy_buffer_to_image(
    vk::Buffer src_buffer, vk::Image dst_image, vk::Extent2D size, vk::CommandBuffer cmd_buffer
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

} // namespace vlk
