#pragma once

#include <array>
#include <concepts>
#include <span>

#include <vulkan/vulkan_raii.hpp>

namespace vlk {

class memory {
public:
  memory() noexcept = default;

  vk::raii::Buffer bind_buffer(const vk::raii::Device& dev,
      vk::BufferUsageFlags usage, vk::DeviceSize start, vk::DeviceSize size);

  const vk::raii::DeviceMemory& get() const noexcept { return mem_; }

  [[nodiscard]] static inline memory alocate(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props, uint32_t type_filter,
      vk::DeviceSize size) {
    return allocate(dev, props, vk::MemoryPropertyFlagBits::eDeviceLocal,
        type_filter, size);
  }

protected:
  explicit memory(vk::raii::DeviceMemory mem) noexcept : mem_{std::move(mem)} {}

  [[nodiscard]] static memory allocate(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props,
      vk::MemoryPropertyFlags flags, uint32_t type_filter, vk::DeviceSize size);

private:
  vk::raii::DeviceMemory mem_ = nullptr;
};

class mappable_memory : public memory {
public:
  mappable_memory() noexcept = default;

  template <std::invocable<std::span<std::byte>> F>
  void with_mapping(vk::DeviceSize offset, vk::DeviceSize size, F&& f) {
    auto deleter = [mem = &get()](std::byte*) { mem->unmapMemory(); };
    std::unique_ptr<std::byte, decltype(deleter)> mapping{
        static_cast<std::byte*>(get().mapMemory(offset, size, {})), deleter};
    std::forward<F>(f)(std::span{mapping.get(), size});
  }

protected:
  explicit mappable_memory(memory mem) noexcept : memory{std::move(mem)} {}
};

class staging_memory : public mappable_memory {
public:
  staging_memory() noexcept = default;

  [[nodiscard]] static staging_memory allocate(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props, vk::DeviceSize size);

private:
  explicit staging_memory(memory mem) noexcept
      : mappable_memory{std::move(mem)} {}
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
      : memory{static_cast<memory&&>(rhs)},
        mapping_{std::exchange(rhs.mapping_, {})} {}

  mapped_memory& operator=(mapped_memory&& rhs) noexcept {
    static_cast<memory&>(*this) = static_cast<memory&&>(rhs);
    mapping_ = std::exchange(rhs.mapping_, {});
    return *this;
  }

  [[nodiscard]] static mapped_memory allocate(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props,
      vk::BufferUsageFlags usage, vk::DeviceSize size);

  void flush(size_t off, size_t len) const {
    get().getDevice().flushMappedMemoryRanges(
        {vk::MappedMemoryRange{.memory = *get(), .offset = off, .size = len}});
  }

  void flush() const {
    get().getDevice().flushMappedMemoryRanges({vk::MappedMemoryRange{
        .memory = *get(), .offset = 0, .size = mapping_.size()}});
  }

  std::span<std::byte> mapping() const noexcept { return mapping_; }
  std::byte* data() const noexcept { return mapping_.data(); }
  size_t size() const noexcept { return mapping_.size(); }

  auto begin() const noexcept { return mapping_.begin(); }
  auto end() const noexcept { return mapping_.end(); }

private:
  explicit mapped_memory(memory mem, size_t sz)
      : memory{std::move(mem)},
        mapping_{static_cast<std::byte*>(get().mapMemory(0, sz)), sz} {}

private:
  std::span<std::byte> mapping_;
};

class memory_pools {
public:
  enum class purpose { vbo, ibo };
  using enum purpose;

  struct sizes {
    size_t vbo_capacity;
    size_t ibo_capacity;
    size_t staging_size;
  };

  memory_pools(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props, sizes pool_sizes);

  vk::raii::Buffer prepare_buffer(const vk::raii::Device& dev,
      const vk::Queue& transfer_queue, const vk::CommandBuffer& cmd, purpose p,
      std::span<const std::byte> data);

private:
  constexpr static size_t purposes_count = 2;

  struct arena_info {
    size_t used;
    size_t capacity;
    size_t alignment;
    uint32_t pool_idx;
  };

private:
  staging_memory staging_mem_;
  size_t staging_size_;
  std::array<arena_info, purposes_count> arena_infos_;
  std::array<memory, purposes_count> pools_;
};

} // namespace vlk
