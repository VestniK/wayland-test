module;

#include <span>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <libs/memtricks/monotonic_arena.hpp>
#include <libs/memtricks/region.hpp>

export module vlk:buf;

export namespace vlk {

class memory {
public:
  memory() noexcept = default;

  vk::raii::Buffer bind_buffer(const vk::raii::Device& dev, vk::BufferUsageFlags usage, memory_region region);

  const vk::raii::DeviceMemory& get() const noexcept { return mem_; }

  [[nodiscard]] static inline memory alocate(
      const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props, uint32_t type_filter,
      vk::DeviceSize size
  ) {
    return allocate(dev, props, vk::MemoryPropertyFlagBits::eDeviceLocal, type_filter, size);
  }

protected:
  explicit memory(vk::raii::DeviceMemory mem) noexcept : mem_{std::move(mem)} {}

  [[nodiscard]] static memory allocate(
      const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props,
      vk::MemoryPropertyFlags flags, uint32_t type_filter, vk::DeviceSize size
  );

private:
  vk::raii::DeviceMemory mem_ = nullptr;
};

class mapped_memory : public memory {
public:
  mapped_memory() noexcept = default;
  ~mapped_memory() noexcept {
    if (*get())
      get().unmapMemory();
  }

  mapped_memory(const mapped_memory&) = delete;
  const mapped_memory& operator=(const mapped_memory&) = delete;

  mapped_memory(mapped_memory&& rhs) noexcept
      : memory{static_cast<memory&&>(rhs)}, mapping_{std::exchange(rhs.mapping_, {})},
        non_coherent_atom_size_{rhs.non_coherent_atom_size_} {}

  mapped_memory& operator=(mapped_memory&& rhs) noexcept {
    static_cast<memory&>(*this) = static_cast<memory&&>(rhs);
    mapping_ = std::exchange(rhs.mapping_, {});
    non_coherent_atom_size_ = rhs.non_coherent_atom_size_;
    return *this;
  }

  using memory::bind_buffer;

  vk::raii::Buffer
  bind_buffer(const vk::raii::Device& dev, vk::BufferUsageFlags usage, std::span<const std::byte> subspan) {
    return bind_buffer(dev, usage, subspan_region(mapping_, subspan));
  }

  [[nodiscard]] static mapped_memory allocate(
      const vk::raii::Device& dev, const vk::PhysicalDeviceMemoryProperties& props,
      const vk::PhysicalDeviceLimits& limits, vk::BufferUsageFlags usage, vk::DeviceSize size
  );

  void flush(memory_region region) const {
    const auto prefix = region.offset % non_coherent_atom_size_;
    region.offset -= prefix;
    region.len = std::min(region.len + prefix, mapping_.size());
    const auto suffix =
        (non_coherent_atom_size_ - region.len % non_coherent_atom_size_) % non_coherent_atom_size_;
    region.len = std::min(region.len + suffix, mapping_.size());

    get().getDevice().flushMappedMemoryRanges(
        {vk::MappedMemoryRange{}.setMemory(*get()).setOffset(region.offset).setSize(region.len)}
    );
  }

  void flush(std::span<const std::byte> subrange) const { flush(subspan_region(mapping_, subrange)); }

  void flush() const {
    get().getDevice().flushMappedMemoryRanges(
        {vk::MappedMemoryRange{}.setMemory(*get()).setSize(mapping_.size())}
    );
  }

  std::span<std::byte> mapping() const noexcept { return mapping_; }
  std::byte* data() const noexcept { return mapping_.data(); }
  size_t size() const noexcept { return mapping_.size(); }

  auto begin() const noexcept { return mapping_.begin(); }
  auto end() const noexcept { return mapping_.end(); }

private:
  explicit mapped_memory(memory mem, size_t sz, const vk::PhysicalDeviceLimits& limits)
      : memory{std::move(mem)}, mapping_{static_cast<std::byte*>(get().mapMemory(0, sz)), sz},
        non_coherent_atom_size_{limits.nonCoherentAtomSize} {}

private:
  std::span<std::byte> mapping_;
  vk::DeviceSize non_coherent_atom_size_ = 0;
};

void copy(vk::Queue transfer_queue, vk::CommandBuffer cmd, vk::Buffer src, vk::Buffer dst, size_t count);

} // namespace vlk

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
