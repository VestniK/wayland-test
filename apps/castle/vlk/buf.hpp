#pragma once

#include <span>

#include <vulkan/vulkan_raii.hpp>

#include <libs/memtricks/monotonic_arena.hpp>
#include <libs/memtricks/region.hpp>

namespace vlk {

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

void copy(vk::Queue transfer_queue, vk::CommandBuffer cmd, vk::Buffer src, vk::Image dst, vk::Extent2D sz);

void copy(vk::Queue transfer_queue, vk::CommandBuffer cmd, vk::Buffer src, vk::Buffer dst, size_t count);

} // namespace vlk
