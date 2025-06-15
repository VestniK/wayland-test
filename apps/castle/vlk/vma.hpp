#pragma once

#include <memory>

#include <vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

namespace vlk {

namespace detail {
struct vma_allocator_delete {
  void operator()(VmaAllocator alloc) const noexcept { vmaDestroyAllocator(alloc); }
};

using vma_allocator_ptr = std::unique_ptr<std::remove_pointer_t<VmaAllocator>, detail::vma_allocator_delete>;
} // namespace detail

template <typename Res>
class allocated_resource {
public:
  allocated_resource() noexcept = default;
  allocated_resource(VmaAllocator alloc, Res res, VmaAllocation mem) noexcept
      : alloc_{alloc}, resource_{res}, mem_{mem} {}

  allocated_resource(const allocated_resource&) = delete;
  allocated_resource& operator=(const allocated_resource&) = delete;

  allocated_resource(allocated_resource&& other) noexcept
      : alloc_{std::exchange(other.alloc_, nullptr)}, resource_{std::exchange(other.resource_, nullptr)},
        mem_{std::exchange(other.mem_, nullptr)} {}

  allocated_resource& operator=(allocated_resource&& other) noexcept {
    if (alloc_)
      clear();
    alloc_ = std::exchange(other.alloc_, nullptr);
    resource_ = std::exchange(other.resource_, nullptr);
    mem_ = std::exchange(other.mem_, nullptr);
  }

  ~allocated_resource() noexcept {
    if (alloc_)
      clear();
  }

  Res resource() const noexcept { return resource_; }
  VmaAllocation allocation() const noexcept { return mem_; }
  VmaAllocator allocator() const noexcept { return alloc_; }

private:
  inline void clear() noexcept {
    if constexpr (std::is_same_v<Res, VkBuffer>)
      vmaDestroyBuffer(alloc_, resource_, mem_);
    else if constexpr (std::is_same_v<Res, VkImage>)
      vmaDestroyImage(alloc_, resource_, mem_);
  }

private:
  VmaAllocator alloc_ = nullptr;
  Res resource_ = nullptr;
  VmaAllocation mem_ = nullptr;
};

class staging_buf : public allocated_resource<VkBuffer> {
public:
  staging_buf(VmaAllocator alloc, VkBuffer res, VmaAllocation mem) noexcept
      : allocated_resource<VkBuffer>{alloc, res, mem} {}

  std::span<std::byte> mapping() const noexcept {
    VmaAllocationInfo info;
    vmaGetAllocationInfo(allocator(), allocation(), &info);
    return {reinterpret_cast<std::byte*>(info.pMappedData), info.size};
  }

  void flush();
};

struct vma_allocator : detail::vma_allocator_ptr {
  vma_allocator() noexcept = default;
  vma_allocator(vk::Instance inst, vk::PhysicalDevice phy_dev, vk::Device dev);

  staging_buf allocate_staging_buffer(size_t size) const;
  allocated_resource<VkImage> allocate_image(vk::Format fmt, vk::Extent2D sz) const;
};

} // namespace vlk
