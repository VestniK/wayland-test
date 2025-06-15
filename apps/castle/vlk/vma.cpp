#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_VULKAN_VERSION 1002000
#include "vk_mem_alloc.h"

#include "vma.hpp"

namespace vlk {

namespace {

auto make_vma_allocator(vk::Instance inst, vk::PhysicalDevice phy_dev, vk::Device dev) {
  VmaVulkanFunctions funcs{
      .vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
      .vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
      .vkGetPhysicalDeviceMemoryProperties =
          VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
      .vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
      .vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
      .vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
      .vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
      .vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
      .vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
      .vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
      .vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
      .vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
      .vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
      .vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
      .vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
      .vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
      .vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
      .vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
      .vkGetBufferMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>(
          dev.getProcAddr("vkGetBufferMemoryRequirements2KHR")
      ),
      .vkGetImageMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(
          dev.getProcAddr("vkGetImageMemoryRequirements2KHR")
      ),
      .vkBindBufferMemory2KHR =
          reinterpret_cast<PFN_vkBindBufferMemory2KHR>(dev.getProcAddr("vkBindBufferMemory2KHR")),
      .vkBindImageMemory2KHR =
          reinterpret_cast<PFN_vkBindImageMemory2KHR>(dev.getProcAddr("vkBindImageMemory2KHR")),
      .vkGetPhysicalDeviceMemoryProperties2KHR =
          reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>(
              inst.getProcAddr("vkGetPhysicalDeviceMemoryProperties2KHR")
          ),
  };
  VmaAllocator alloc = nullptr;
  VmaAllocatorCreateInfo allocator_info = {};
  allocator_info.physicalDevice = phy_dev;
  allocator_info.device = dev;
  allocator_info.instance = inst;
  allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  allocator_info.pVulkanFunctions = &funcs;
  const auto res = static_cast<vk::Result>(vmaCreateAllocator(&allocator_info, &alloc));

  if (const auto ec = vk::make_error_code(res))
    throw std::system_error(ec, "vmaCreateAllocator");

  return detail::vma_allocator_ptr{alloc};
}

} // namespace

void staging_buf::flush() {
  const auto ec =
      make_error_code(static_cast<vk::Result>(vmaFlushAllocation(allocator(), allocation(), 0, VK_WHOLE_SIZE))
      );
  if (ec)
    throw std::system_error{ec, "vmaFlushAllocation"};
}

vma_allocator::vma_allocator(vk::Instance inst, vk::PhysicalDevice phy_dev, vk::Device dev)
    : detail::vma_allocator_ptr{make_vma_allocator(inst, phy_dev, dev)} {}

staging_buf vma_allocator::allocate_staging_buffer(size_t size) const {
  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
  alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  const VkBufferCreateInfo buf_inf =
      vk::BufferCreateInfo{}.setSize(size).setUsage(vk::BufferUsageFlagBits::eTransferSrc);

  VkBuffer buf;
  VmaAllocation mem;

  const auto ec = make_error_code(
      static_cast<vk::Result>(vmaCreateBuffer(get(), &buf_inf, &alloc_info, &buf, &mem, nullptr))
  );
  if (ec)
    throw std::system_error{ec, "vmaCreateBuffer"};

  return {get(), buf, mem};
}

allocated_resource<VkImage> vma_allocator::allocate_image(vk::Format fmt, vk::Extent2D sz) const {
  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

  VkImageCreateInfo img_inf =
      vk::ImageCreateInfo{}
          .setImageType(vk::ImageType::e2D)
          .setFormat(fmt)
          .setExtent(vk::Extent3D{sz, 1})
          .setMipLevels(1)
          .setArrayLayers(1)
          .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

  VkImage img;
  VmaAllocation mem;
  const auto ec = make_error_code(
      static_cast<vk::Result>(vmaCreateImage(get(), &img_inf, &alloc_info, &img, &mem, nullptr))
  );
  if (ec)
    throw std::system_error{ec, "vmaCreateBuffer"};

  return {get(), img, mem};
}

} // namespace vlk
