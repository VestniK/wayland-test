#include "vlk_buf.hpp"

#include <vulkan/vulkan.hpp>

namespace vlk {

namespace {

uint32_t choose_mem_type(uint32_t type_filter,
    const vk::PhysicalDeviceMemoryProperties& mem_props,
    vk::MemoryPropertyFlags mem_flags) {
  for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
    if ((type_filter & (1 << i)) &&
        (mem_props.memoryTypes[i].propertyFlags & mem_flags) == mem_flags) {
      return i;
    }
  }
  throw std::runtime_error{"Failed to find suitable GPU memory type"};
}

vk::BufferUsageFlags purpose_to_usage(memory_pools::purpose p) {
  vk::BufferUsageFlags buf_usage;
  switch (p) {
  case memory_pools::vbo:
    buf_usage = vk::BufferUsageFlagBits::eVertexBuffer |
                vk::BufferUsageFlagBits::eTransferDst;
    break;
  case memory_pools::ibo:
    buf_usage = vk::BufferUsageFlagBits::eIndexBuffer |
                vk::BufferUsageFlagBits::eTransferDst;
    break;
  }
  return buf_usage;
}

vk::BufferCreateInfo make_bufer_create_info(
    vk::BufferUsageFlags usage, size_t sz) {
  return vk::BufferCreateInfo{.size = sz,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive,
      .queueFamilyIndexCount = {},
      .pQueueFamilyIndices = {}};
}

vk::MemoryRequirements query_memreq(
    const vk::Device& dev, vk::BufferUsageFlags usage) {
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
  const vk::BufferCreateInfo buf_create_info =
      make_bufer_create_info(usage, dummy_size);
  return dev.getBufferMemoryRequirementsKHR({.pCreateInfo = &buf_create_info})
      .memoryRequirements;
}

} // namespace

vk::raii::Buffer memory::bind_buffer(const vk::raii::Device& dev,
    vk::BufferUsageFlags usage, vk::DeviceSize start, vk::DeviceSize size) {
  vk::raii::Buffer res{dev, make_bufer_create_info(usage, size)};
  res.bindMemory(*mem_, start);
  return res;
}

memory memory::allocate(const vk::raii::Device& dev,
    const vk::PhysicalDeviceMemoryProperties& props,
    vk::MemoryPropertyFlags flags, uint32_t type_filter, vk::DeviceSize size) {
  return memory{
      dev.allocateMemory(vk::MemoryAllocateInfo{.allocationSize = size,
          .memoryTypeIndex = choose_mem_type(type_filter, props, flags)})};
}

[[nodiscard]] staging_memory staging_memory::allocate(
    const vk::raii::Device& dev,
    const vk::PhysicalDeviceMemoryProperties& props, vk::DeviceSize size) {
  return staging_memory{memory::allocate(dev, props,
      vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent,
      query_memreq(*dev, vk::BufferUsageFlagBits::eTransferSrc).memoryTypeBits,
      size)};
}

[[nodiscard]] uniform_memory uniform_memory::allocate(
    const vk::raii::Device& dev,
    const vk::PhysicalDeviceMemoryProperties& props, vk::DeviceSize size) {
  return uniform_memory{
      memory::allocate(dev, props, vk::MemoryPropertyFlagBits::eHostVisible,
          query_memreq(*dev, vk::BufferUsageFlagBits::eUniformBuffer)
              .memoryTypeBits,
          size)};
}

memory_pools::memory_pools(const vk::raii::Device& dev,
    const vk::PhysicalDeviceMemoryProperties& props, sizes pool_sizes)
    : staging_mem_{staging_memory::allocate(
          dev, props, pool_sizes.staging_size)},
      staging_size_{pool_sizes.staging_size} {
  std::array<std::tuple<vk::MemoryRequirements, purpose, size_t>,
      purposes_count>
      type2purpose{std::tuple{query_memreq(*dev, purpose_to_usage(vbo)), vbo,
                       pool_sizes.vbo_capacity},
          std::tuple{query_memreq(*dev, purpose_to_usage(ibo)), ibo,
              pool_sizes.ibo_capacity}};

  std::ranges::sort(type2purpose, [](auto l, auto r) {
    return std::get<0>(l).memoryTypeBits < std::get<0>(r).memoryTypeBits;
  });
  std::optional<uint32_t> last_type_bits;
  uint32_t next_mem_slot_idx = 0;
  size_t total_capacity = 0;
  for (auto [mem_req, purp, capacity] : type2purpose) {
    if (const auto prev = std::exchange(last_type_bits, mem_req.memoryTypeBits);
        prev && prev != mem_req.memoryTypeBits) {
      pools_[next_mem_slot_idx++] = memory::alocate(
          dev, props, mem_req.memoryTypeBits, std::exchange(total_capacity, 0));
    }
    total_capacity += capacity;
    arena_infos_[static_cast<size_t>(purp)] = {
        .used = 0,
        .capacity = capacity,
        .alignment = mem_req.alignment,
        .pool_idx = next_mem_slot_idx,
    };
  }
  pools_[next_mem_slot_idx] =
      memory::alocate(dev, props, *last_type_bits, total_capacity);
}

vk::raii::Buffer memory_pools::prepare_buffer(const vk::raii::Device& dev,
    const vk::Queue& transfer_queue, const vk::CommandBuffer& cmd, purpose p,
    std::span<const std::byte> data) {
  if (staging_size_ < data.size())
    throw std::bad_alloc{};
  staging_mem_.with_mapping(
      0, data.size(), [data](std::span<std::byte> mapped) {
        std::ranges::copy(data, mapped.data());
      });
  vk::raii::Buffer staging_buf = staging_mem_.bind_buffer(
      dev, vk::BufferUsageFlagBits::eTransferSrc, 0, data.size());

  arena_info& arena = arena_infos_[static_cast<size_t>(p)];
  const size_t offset = std::ranges::fold_left(
      arena_infos_, size_t{0}, [&arena](size_t accum, const arena_info& item) {
        return accum + (item.pool_idx == arena.pool_idx ? item.used : 0);
      });
  const size_t padding =
      (arena.alignment - offset % arena.alignment) % arena.alignment;
  if (arena.capacity - arena.used < data.size() + padding)
    throw std::bad_alloc{};
  arena.used = arena.used + data.size() + padding;
  auto res = pools_[arena.pool_idx].bind_buffer(
      dev, purpose_to_usage(p), offset + padding, data.size());

  cmd.begin(
      vk::CommandBufferBeginInfo{.flags = {}, .pInheritanceInfo = nullptr});
  cmd.copyBuffer(*staging_buf, *res,
      vk::BufferCopy{.srcOffset = 0, .dstOffset = 0, .size = data.size()});
  cmd.end();

  transfer_queue.submit({vk::SubmitInfo{.waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr}});
  transfer_queue.waitIdle();

  return res;
}

} // namespace vlk
