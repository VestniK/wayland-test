#include "buf.hpp"

#include <vulkan/vulkan.hpp>

#include "arena.impl.hpp"

namespace vlk {

namespace {

uint32_t choose_mem_type(
    uint32_t type_filter, const vk::PhysicalDeviceMemoryProperties& mem_props,
    vk::MemoryPropertyFlags mem_flags
) {
  for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
    if ((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & mem_flags) == mem_flags) {
      return i;
    }
  }
  throw std::runtime_error{"Failed to find suitable GPU memory type"};
}

vk::BufferUsageFlags purpose_to_usage(buffer_purpose p) {
  vk::BufferUsageFlags buf_usage;
  switch (p) {
  case memory_pools::vbo:
    buf_usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    break;
  case memory_pools::ibo:
    buf_usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    break;
  }
  return buf_usage;
}

vk::ImageUsageFlags purpose_to_usage(image_purpose p) noexcept {
  vk::ImageUsageFlags buf_usage;
  switch (p) {
  case memory_pools::texture:
    buf_usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    break;
  }
  return buf_usage;
}

constexpr vk::AccessFlags purpose_access_mask(image_purpose p) noexcept {
  vk::AccessFlags res;
  switch (p) {
  case image_purpose::texture:
    res = vk::AccessFlagBits::eShaderRead;
    break;
  }
  return res;
}

constexpr vk::PipelineStageFlags purpose_pipeline_stage(image_purpose p) noexcept {
  vk::PipelineStageFlags res;
  switch (p) {
  case image_purpose::texture:
    res = vk::PipelineStageFlagBits::eFragmentShader;
    break;
  }
  return res;
}

vk::BufferCreateInfo make_bufer_create_info(vk::BufferUsageFlags usage, size_t sz) noexcept {
  return vk::BufferCreateInfo{}.setSize(sz).setUsage(usage);
}

vk::ImageCreateInfo
make_image_create_info(vk::ImageUsageFlags usage, vk::Format fmt, vk::Extent2D sz) noexcept {
  return vk::ImageCreateInfo{}
      .setImageType(vk::ImageType::e2D)
      .setFormat(fmt)
      .setExtent(vk::Extent3D{sz, 1})
      .setMipLevels(1)
      .setArrayLayers(1)
      .setUsage(usage);
}

vk::MemoryRequirements query_memreq(const vk::Device& dev, vk::BufferUsageFlags usage) {
  // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap12.html#VkMemoryRequirements
  // > The implementation guarantees certain properties about the memory
  // > requirements returned by vkGetBufferMemoryRequirements2,
  // > vkGetImageMemoryRequirements2, vkGetDeviceBufferMemoryRequirements,
  // > vkGetDeviceImageMemoryRequirements, vkGetBufferMemoryRequirements and
  // > vkGetImageMemoryRequirements:
  // > ...
  // > The memoryTypeBits member is identical for all VkBuffer objects created
  // > with the same value for the flags and usage members in the
  // > VkBufferCreateInfo structure and the handleTypes member of the
  // > VkExternalMemoryBufferCreateInfo structure passed to vkCreateBuffer.
  //
  // This merelly means that size can be anything for querying memoryTypeBit
  constexpr vk::DeviceSize dummy_size = 100500;
  const vk::BufferCreateInfo buf_create_info = make_bufer_create_info(usage, dummy_size);
  return dev
      .getBufferMemoryRequirementsKHR(vk::DeviceBufferMemoryRequirements{}.setPCreateInfo(&buf_create_info))
      .memoryRequirements;
}

vk::MemoryRequirements query_memreq(const vk::Device& dev, vk::ImageUsageFlags usage) {
  // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap12.html#VkMemoryRequirements
  //
  // If the maintenance4 feature is enabled, then the alignment member is
  // identical for all VkImage objects created with the same combination of
  // values for the flags, imageType, format, extent, mipLevels, arrayLayers,
  // samples, tiling and usage members in the VkImageCreateInfo structure passed
  // to vkCreateImage.
  //
  // For images created with a color format, the memoryTypeBits member is
  // identical for all VkImage objects created with the same combination of
  // values for the tiling member, the VK_IMAGE_CREATE_SPARSE_BINDING_BIT bit
  // and VK_IMAGE_CREATE_PROTECTED_BIT bit of the flags member, the
  // VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT bit of the flags member,
  // the VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT bit of the usage member if the
  // VkPhysicalDeviceHostImageCopyPropertiesEXT::identicalMemoryTypeRequirements
  // property is VK_FALSE, handleTypes member of
  // VkExternalMemoryImageCreateInfo, and the
  // VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT of the usage member in the
  // VkImageCreateInfo structure passed to vkCreateImage.
  //
  // This means that alignment requiremetns of arena are useless for images
  // because different pixel formats and even diferent extents might affect
  // exact image allignment requirements.
  const vk::ImageCreateInfo img_create_info = make_image_create_info(usage, vk::Format::eR8G8B8A8Srgb, {});
  return dev
      .getImageMemoryRequirementsKHR(vk::DeviceImageMemoryRequirements{}.setPCreateInfo(&img_create_info))
      .memoryRequirements;
}

} // namespace

vk::raii::Buffer
memory::bind_buffer(const vk::raii::Device& dev, vk::BufferUsageFlags usage, memory_region region) {
  vk::raii::Buffer res{dev, make_bufer_create_info(usage, region.len)};
  res.bindMemory(*mem_, region.offset);
  return res;
}

memory memory::allocate(
    const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props,
    vk::MemoryPropertyFlags flags, uint32_t type_filter, vk::DeviceSize size
) {
  return memory{dev.allocateMemory(vk::MemoryAllocateInfo{}.setAllocationSize(size).setMemoryTypeIndex(
      choose_mem_type(type_filter, props, flags)
  ))};
}

[[nodiscard]] mapped_memory mapped_memory::allocate(
    const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props,
    const vk::PhysicalDeviceLimits& limits, vk::BufferUsageFlags usage, vk::DeviceSize size
) {
  return mapped_memory{
      memory::allocate(
          dev, props, vk::MemoryPropertyFlagBits::eHostVisible, query_memreq(*dev, usage).memoryTypeBits, size
      ),
      size, limits
  };
}

memory_pools::memory_pools(
    const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props,
    const vk::PhysicalDeviceLimits& limits, detail::sizes pool_sizes
)
    : staging_mem_{mapped_memory::allocate(
          dev, props, limits, vk::BufferUsageFlagBits::eTransferSrc, pool_sizes.staging_size
      )},
      arenas_{
          pool_sizes, [&dev](auto purpose) { return query_memreq(*dev, purpose_to_usage(purpose)); },
          [&dev, &props](uint32_t memoryTypeBits, size_t size) {
            return memory::alocate(dev, props, memoryTypeBits, size);
          }
      } {}

vk::raii::Buffer memory_pools::prepare_buffer(
    const vk::raii::Device& dev, const vk::Queue& transfer_queue, const vk::CommandBuffer& cmd,
    buffer_purpose p, staging_memory data
) {
  assert(data.get_deleter().parent() == this);
  staging_mem_.mem_source().flush(data);
  vk::raii::Buffer staging_buf =
      staging_mem_.mem_source().bind_buffer(dev, vk::BufferUsageFlagBits::eTransferSrc, data);

  auto [arena_memory, region] = arenas_.lock_memory_for(p, data.size());
  auto res = arena_memory.bind_buffer(dev, purpose_to_usage(p), region);

  cmd.begin(vk::CommandBufferBeginInfo{});
  cmd.copyBuffer(*staging_buf, *res, vk::BufferCopy{}.setSize(data.size()));
  cmd.end();

  transfer_queue.submit(vk::SubmitInfo{}.setCommandBuffers(cmd));
  transfer_queue.waitIdle();

  return res;
}

void copy(
    vk::Queue transfer_queue, vk::CommandBuffer cmd, vk::Buffer src, vk::Image dst, image_purpose p,
    vk::Extent2D sz
) {
  cmd.begin(vk::CommandBufferBeginInfo{});

  const auto img_dst_barrier = vk::ImageMemoryBarrier{}
                                   .setSrcAccessMask(vk::AccessFlagBits::eNone)
                                   .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
                                   .setOldLayout(vk::ImageLayout::eUndefined)
                                   .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                                   .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                   .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                   .setImage(dst)
                                   .setSubresourceRange(vk::ImageSubresourceRange{}
                                                            .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                            .setLevelCount(1)
                                                            .setLayerCount(1));
  cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, img_dst_barrier
  );

  auto copy_region =
      vk::BufferImageCopy{}
          .setImageExtent(vk::Extent3D{sz, 1})
          .setImageSubresource(
              vk::ImageSubresourceLayers{}.setLayerCount(1).setAspectMask(vk::ImageAspectFlagBits::eColor)
          );
  cmd.copyBufferToImage(src, dst, vk::ImageLayout::eTransferDstOptimal, copy_region);

  const auto img_sampler_barrier =
      vk::ImageMemoryBarrier{}
          .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
          .setDstAccessMask(purpose_access_mask(p))
          .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
          .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
          .setImage(dst)
          .setSubresourceRange(vk::ImageSubresourceRange{}
                                   .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                   .setLevelCount(1)
                                   .setLayerCount(1));
  cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer, purpose_pipeline_stage(p), {}, {}, {}, img_sampler_barrier
  );

  cmd.end();

  transfer_queue.submit(vk::SubmitInfo{}.setCommandBuffers(cmd));
  transfer_queue.waitIdle();
}

} // namespace vlk
