#pragma once

#include <array>
#include <concepts>
#include <span>

#include <vulkan/vulkan_raii.hpp>

#include <libs/memtricks/monotonic_arena.hpp>

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
        mapping_{std::exchange(rhs.mapping_, {})},
        non_coherent_atom_size_{rhs.non_coherent_atom_size_} {}

  mapped_memory& operator=(mapped_memory&& rhs) noexcept {
    static_cast<memory&>(*this) = static_cast<memory&&>(rhs);
    mapping_ = std::exchange(rhs.mapping_, {});
    non_coherent_atom_size_ = rhs.non_coherent_atom_size_;
    return *this;
  }

  [[nodiscard]] static mapped_memory allocate(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props,
      const vk::PhysicalDeviceLimits& limits, vk::BufferUsageFlags usage,
      vk::DeviceSize size);

  void flush(size_t off, size_t len) const {
    const auto prefix = off % non_coherent_atom_size_;
    off -= prefix;
    len = std::min(len + prefix, mapping_.size());
    const auto suffix =
        (non_coherent_atom_size_ - len % non_coherent_atom_size_) %
        non_coherent_atom_size_;
    len = std::min(len + suffix, mapping_.size());

    get().getDevice().flushMappedMemoryRanges(
        {vk::MappedMemoryRange{.memory = *get(), .offset = off, .size = len}});
  }

  void flush(std::span<const std::byte> subrange) const {
    flush(subrange.data() - mapping_.data(), subrange.size());
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
  explicit mapped_memory(
      memory mem, size_t sz, const vk::PhysicalDeviceLimits& limits)
      : memory{std::move(mem)},
        mapping_{static_cast<std::byte*>(get().mapMemory(0, sz)), sz},
        non_coherent_atom_size_{limits.nonCoherentAtomSize} {}

private:
  std::span<std::byte> mapping_;
  vk::DeviceSize non_coherent_atom_size_ = 0;
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

  class staging_deleter {
  public:
    explicit staging_deleter(size_t sz, memory_pools* parent) noexcept
        : size_{sz}, parent_{parent} {}

    void operator()(const std::byte* ptr) const noexcept {
      parent_->deallocate_staging({ptr, size_});
    }

    size_t size() const noexcept { return size_; }

  private:
    size_t size_ = 0;
    memory_pools* parent_ = nullptr;
  };

  struct staging_memory : std::unique_ptr<std::byte[], staging_deleter> {
    staging_memory(std::byte* ptr, staging_deleter del) noexcept
        : std::unique_ptr<std::byte[], staging_deleter>{ptr, del} {}

    std::byte* data() const noexcept { return get(); }
    size_t size() const noexcept { return get_deleter().size(); }

    operator std::span<std::byte>() const noexcept { return {data(), size()}; }
    operator std::span<const std::byte>() const noexcept {
      return {data(), size()};
    }
  };

public:
  memory_pools(const vk::raii::Device& dev,
      const vk::PhysicalDeviceMemoryProperties& props,
      const vk::PhysicalDeviceLimits& limits, sizes pool_sizes);

  staging_memory allocate_staging_memory(size_t size) {
    ++allocations_counter_;
    return staging_memory{static_cast<std::byte*>(staging_mem_.allocate(size)),
        staging_deleter{size, this}};
  }

  staging_memory allocate_staging_memory(std::span<const std::byte> data) {
    auto res = allocate_staging_memory(data.size());
    std::ranges::copy(data, res.data());
    return res;
  }

  vk::raii::Buffer prepare_buffer(const vk::raii::Device& dev,
      const vk::Queue& transfer_queue, const vk::CommandBuffer& cmd, purpose p,
      staging_memory data);

private:
  void deallocate_staging(std::span<const std::byte> data) noexcept {
    if (--allocations_counter_ == 0)
      staging_mem_.clear();
  }

private:
  constexpr static size_t purposes_count = 2;

  struct arena_info {
    size_t used;
    size_t capacity;
    size_t alignment;
    uint32_t pool_idx;
  };

private:
  monotonic_arena<mapped_memory> staging_mem_;
  unsigned allocations_counter_ = 0;
  std::array<arena_info, purposes_count> arena_infos_;
  std::array<memory, purposes_count> pools_;
};

} // namespace vlk
