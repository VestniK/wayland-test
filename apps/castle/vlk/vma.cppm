module;

#include <memory>

#include "vk_mem_alloc.h"

#include <vulkan/vulkan.hpp>

export module vlk.vma;

namespace vlk {

namespace detail {
struct vma_allocator_delete {
  void operator()(VmaAllocator alloc) const noexcept { vmaDestroyAllocator(alloc); }
};

using vma_allocator_ptr = std::unique_ptr<std::remove_pointer_t<VmaAllocator>, detail::vma_allocator_delete>;
} // namespace detail

export template <typename Res>
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
    if constexpr (std::is_same_v<Res, vk::Buffer>)
      vmaDestroyBuffer(alloc_, resource_, mem_);
    else if constexpr (std::is_same_v<Res, vk::Image>)
      vmaDestroyImage(alloc_, resource_, mem_);
  }

private:
  VmaAllocator alloc_ = nullptr;
  Res resource_ = nullptr;
  VmaAllocation mem_ = nullptr;
};

export class staging_buf : public allocated_resource<vk::Buffer> {
public:
  staging_buf(VmaAllocator alloc, VkBuffer res, VmaAllocation mem) noexcept
      : allocated_resource<vk::Buffer>{alloc, res, mem} {}

  std::span<std::byte> mapping() const noexcept {
    VmaAllocationInfo info;
    vmaGetAllocationInfo(allocator(), allocation(), &info);
    return {reinterpret_cast<std::byte*>(info.pMappedData), info.size};
  }

  void flush();
};

export class vma_allocator {
public:
  using owned = detail::vma_allocator_ptr;

  vma_allocator() noexcept = default;
  explicit vma_allocator(const owned& owner) noexcept : alloc_{owner.get()} {}

  static owned create(vk::Instance inst, vk::PhysicalDevice phy_dev, vk::Device dev);

  staging_buf allocate_staging_buffer(size_t size) const;
  allocated_resource<vk::Buffer> allocate_buffer(vk::BufferUsageFlags usage, size_t count) const;
  allocated_resource<vk::Image> allocate_image(vk::Format fmt, vk::Extent2D sz) const;

private:
  VmaAllocator alloc_ = nullptr;
};

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
  const auto ec = make_error_code(
      static_cast<vk::Result>(vmaFlushAllocation(allocator(), allocation(), 0, VK_WHOLE_SIZE))
  );
  if (ec)
    throw std::system_error{ec, "vmaFlushAllocation"};
}

vma_allocator::owned vma_allocator::create(vk::Instance inst, vk::PhysicalDevice phy_dev, vk::Device dev) {
  return owned{make_vma_allocator(inst, phy_dev, dev)};
}

staging_buf vma_allocator::allocate_staging_buffer(size_t size) const {
  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
  alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  const VkBufferCreateInfo buf_inf =
      vk::BufferCreateInfo{}.setSize(size).setUsage(vk::BufferUsageFlagBits::eTransferSrc);

  VkBuffer buf;
  VmaAllocation mem;

  const auto ec = make_error_code(
      static_cast<vk::Result>(vmaCreateBuffer(alloc_, &buf_inf, &alloc_info, &buf, &mem, nullptr))
  );
  if (ec)
    throw std::system_error{ec, "vmaCreateBuffer"};

  return {alloc_, buf, mem};
}

allocated_resource<vk::Buffer>
vma_allocator::allocate_buffer(vk::BufferUsageFlags usage, size_t count) const {
  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

  VkBufferCreateInfo buf_inf = vk::BufferCreateInfo{}.setSize(count).setUsage(usage);

  VkBuffer buf;
  VmaAllocation mem;

  const auto ec = make_error_code(
      static_cast<vk::Result>(vmaCreateBuffer(alloc_, &buf_inf, &alloc_info, &buf, &mem, nullptr))
  );
  if (ec)
    throw std::system_error{ec, "vmaCreateBuffer"};

  return {alloc_, buf, mem};
}

allocated_resource<vk::Image> vma_allocator::allocate_image(vk::Format fmt, vk::Extent2D sz) const {
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
      static_cast<vk::Result>(vmaCreateImage(alloc_, &img_inf, &alloc_info, &img, &mem, nullptr))
  );
  if (ec)
    throw std::system_error{ec, "vmaCreateBuffer"};

  return {alloc_, img, mem};
}

} // namespace vlk
