#include "buf.hpp"

#include <vulkan/vulkan.hpp>

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

vk::BufferCreateInfo make_bufer_create_info(vk::BufferUsageFlags usage, size_t sz) noexcept {
  return vk::BufferCreateInfo{}.setSize(sz).setUsage(usage);
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
  return memory{dev.allocateMemory(
      vk::MemoryAllocateInfo{}.setAllocationSize(size).setMemoryTypeIndex(
          choose_mem_type(type_filter, props, flags)
      )
  )};
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

void copy(vk::Queue transfer_queue, vk::CommandBuffer cmd, vk::Buffer src, vk::Buffer dst, size_t count) {
  cmd.begin(vk::CommandBufferBeginInfo{});
  cmd.copyBuffer(src, dst, vk::BufferCopy{}.setSize(count));
  cmd.end();

  transfer_queue.submit(vk::SubmitInfo{}.setCommandBuffers(cmd));
  transfer_queue.waitIdle();
}

} // namespace vlk
