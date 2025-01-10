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
  return vk::BufferCreateInfo{
      .size = sz,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
      .queueFamilyIndexCount = {},
      .pQueueFamilyIndices = {}
  };
}

vk::ImageCreateInfo
make_image_create_info(vk::ImageUsageFlags usage, vk::Format fmt, vk::Extent2D sz) noexcept {
  return {
      .flags = {},
      .imageType = vk::ImageType::e2D,
      .format = fmt,
      .extent = {.width = sz.width, .height = sz.height, .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = vk::SampleCountFlagBits::e1,
      .tiling = vk::ImageTiling::eOptimal,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = vk::ImageLayout::eUndefined,
  };
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
  return dev.getBufferMemoryRequirementsKHR({.pCreateInfo = &buf_create_info}).memoryRequirements;
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
      .getImageMemoryRequirementsKHR(vk::DeviceImageMemoryRequirements{
          .pCreateInfo = &img_create_info, .planeAspect = vk::ImageAspectFlagBits::eColor
      })
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
  return memory{dev.allocateMemory(vk::MemoryAllocateInfo{
      .allocationSize = size, .memoryTypeIndex = choose_mem_type(type_filter, props, flags)
  })};
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

  cmd.begin(vk::CommandBufferBeginInfo{.flags = {}, .pInheritanceInfo = nullptr});
  cmd.copyBuffer(*staging_buf, *res, vk::BufferCopy{.srcOffset = 0, .dstOffset = 0, .size = data.size()});
  cmd.end();

  transfer_queue.submit({vk::SubmitInfo{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
  }});
  transfer_queue.waitIdle();

  return res;
}

vk::raii::Image memory_pools::prepare_image(
    const vk::raii::Device& dev, const vk::Queue& transfer_queue, const vk::CommandBuffer& cmd,
    image_purpose p, staging_memory data, vk::Format fmt, vk::Extent2D sz
) {
  assert(data.get_deleter().parent() == this);
  staging_mem_.mem_source().flush(data);
  vk::raii::Buffer staging_buf =
      staging_mem_.mem_source().bind_buffer(dev, vk::BufferUsageFlagBits::eTransferSrc, data);

  auto res = dev.createImage(make_image_create_info(purpose_to_usage(p), fmt, sz));
  const auto req = (*dev).getImageMemoryRequirements(*res);

  auto [arena_memory, region] = arenas_.lock_memory_for(p, req.size, req.alignment);
  res.bindMemory(arena_memory.get(), region.offset);

  cmd.begin(vk::CommandBufferBeginInfo{.flags = {}, .pInheritanceInfo = nullptr});

  const vk::ImageMemoryBarrier img_dst_barrier{
      .srcAccessMask = vk::AccessFlagBits::eNone,
      .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
      .oldLayout = vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eTransferDstOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = *res,
      .subresourceRange =
          {.aspectMask = vk::ImageAspectFlagBits::eColor,
           .baseMipLevel = 0,
           .levelCount = 1,
           .baseArrayLayer = 0,
           .layerCount = 1}
  };
  cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, img_dst_barrier
  );

  vk::BufferImageCopy regions[] = {vk::BufferImageCopy{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource =
          {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1
          },
      .imageOffset = {},
      .imageExtent = {.width = sz.width, .height = sz.height, .depth = 1}
  }};
  cmd.copyBufferToImage(staging_buf, res, vk::ImageLayout::eTransferDstOptimal, regions);

  const vk::ImageMemoryBarrier img_sampler_barrier{
      .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
      .dstAccessMask = purpose_access_mask(p),
      .oldLayout = vk::ImageLayout::eTransferDstOptimal,
      .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = *res,
      .subresourceRange =
          {.aspectMask = vk::ImageAspectFlagBits::eColor,
           .baseMipLevel = 0,
           .levelCount = 1,
           .baseArrayLayer = 0,
           .layerCount = 1}
  };
  cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer, purpose_pipeline_stage(p), {}, {}, {}, img_sampler_barrier
  );

  cmd.end();

  transfer_queue.submit({vk::SubmitInfo{
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
  }});
  transfer_queue.waitIdle();

  return res;
}

} // namespace vlk
