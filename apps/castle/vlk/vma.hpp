#pragma once

#include <memory>

#include <vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

namespace vlk {

namespace detail {
struct vma_allocator_delete {
  void operator()(VmaAllocator alloc) const noexcept { vmaDestroyAllocator(alloc); }
};
} // namespace detail

using vma_allocator = std::unique_ptr<std::remove_pointer_t<VmaAllocator>, detail::vma_allocator_delete>;

vma_allocator make_vma_allocator(vk::Instance inst, vk::PhysicalDevice phy_dev, vk::Device dev);

} // namespace vlk
