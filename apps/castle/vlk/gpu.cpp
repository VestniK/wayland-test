#include "gpu.hpp"

#include <algorithm>
#include <ranges>

#include <spdlog/spdlog.h>

#include <libs/memtricks/projections.hpp>
#include <libs/memtricks/sorted_array.hpp>

namespace vlk {
namespace {

constexpr auto required_device_extensions = make_sorted_array<const char*>(
    std::less<std::string_view>{}, VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE_4_EXTENSION_NAME
);

vk::raii::Device create_logical_device(const vk::raii::PhysicalDevice& dev, device_queue_families families) {
  const float queue_prio = 1.0f;
  std::array<vk::DeviceQueueCreateInfo, 2> device_queues{
      vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex(families.graphics).setQueuePriorities(queue_prio),
      vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex(families.presentation).setQueuePriorities(queue_prio),
  };

  constexpr auto features = vk::PhysicalDeviceFeatures{}.setSamplerAnisotropy(true);

  auto device_create_info = vk::DeviceCreateInfo{}
                                .setQueueCreateInfoCount(
                                    families.graphics == families.presentation ? 1u : 2u
                                ) // TODO: better duplication needed
                                .setPQueueCreateInfos(device_queues.data())
                                .setPEnabledExtensionNames(required_device_extensions)
                                .setPEnabledFeatures(&features);

  vk::raii::Device device{dev, device_create_info};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
  return device;
}

bool has_required_extensions(vk::PhysicalDevice dev) {
  spdlog::debug(
      "Checking required extensions on suitable device {}", std::string_view{dev.getProperties().deviceName}
  );

  std::vector<vk::ExtensionProperties> exts = dev.enumerateDeviceExtensionProperties();
  std::ranges::sort(exts, std::less<>{}, &vk::ExtensionProperties::extensionName);

  std::vector<std::string_view> missing_exts;
  std::ranges::set_difference(
      required_device_extensions,
      exts | std::views::transform(as_string_view<&vk::ExtensionProperties::extensionName>),
      std::back_inserter(missing_exts), std::less<std::string_view>{}
  );
  for (auto ext : missing_exts)
    spdlog::debug("\tMissing required extension {}", ext);

  return missing_exts.empty();
}

bool check_device(vk::PhysicalDevice dev) {
  if (dev.getProperties().apiVersion < VK_API_VERSION_1_2)
    return false;
  if (!dev.getFeatures().samplerAnisotropy)
    return false;

  return has_required_extensions(dev);
}

bool check_suitable_presentation_params(vk::PhysicalDevice dev, vk::SurfaceKHR surf) {
  if (!std::ranges::any_of(dev.getSurfaceFormatsKHR(surf), [](vk::SurfaceFormatKHR fmt) {
        return fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
               (fmt.format == vk::Format::eR8G8B8A8Srgb || fmt.format == vk::Format::eB8G8R8A8Srgb);
      }))
    return false;

  if (!std::ranges::contains(dev.getSurfacePresentModesKHR(surf), vk::PresentModeKHR::eMailbox))
    return false;

  return true;
}

vk::SwapchainCreateInfoKHR
make_swapchain_create_info(vk::PhysicalDevice dev, vk::SurfaceKHR surf, vk::Format img_fmt, vk::Extent2D sz) {
  const auto capabilities = dev.getSurfaceCapabilitiesKHR(surf);
  return vk::SwapchainCreateInfoKHR{}
      .setSurface(surf)
      .setMinImageCount(std::min(
          capabilities.minImageCount + 1,
          capabilities.maxImageCount == 0 ? std::numeric_limits<uint32_t>::max() : capabilities.maxImageCount
      ))
      .setImageFormat(img_fmt)
      .setImageExtent(vk::Extent2D{
          std::clamp(sz.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
          std::clamp(sz.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
      })
      .setImageArrayLayers(1)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setPreTransform(capabilities.currentTransform)
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
      .setPresentMode(vk::PresentModeKHR::eMailbox)
      .setClipped(true);
}

} // namespace

std::optional<device_queue_families>
device_queue_families::find(vk::PhysicalDevice dev, vk::SurfaceKHR surf) {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> presentation_family;
  for (const auto& [idx, queue_family] : std::views::enumerate(dev.getQueueFamilyProperties())) {
    if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
      graphics_family = idx;
    if (dev.getSurfaceSupportKHR(idx, surf))
      presentation_family = idx;
  }
  if (!graphics_family || !presentation_family)
    return std::nullopt;
  return device_queue_families{.graphics = *graphics_family, .presentation = *presentation_family};
}

gpu::gpu(vk::raii::Instance&& inst, vk::raii::PhysicalDevice&& dev, device_queue_families families)
    : instance_{std::move(inst)}, phydev_{std::move(dev)}, device_{create_logical_device(phydev_, families)},
      families_{families}, alloc_{*instance_, *phydev_, *device_} {}

vk::SampleCountFlagBits gpu::find_max_usable_samples() const noexcept {
  using enum vk::SampleCountFlagBits;
  constexpr vk::SampleCountFlagBits prio_order[] = {e64, e32, e16, e8, e4, e2};

  const auto counts_mask = phydev_.getProperties().limits.framebufferColorSampleCounts;
  const auto it = std::ranges::find_if(prio_order, [&](auto e) { return bool{e & counts_mask}; });
  return it != std::end(prio_order) ? *it : e1;
}

std::optional<vk::Format> gpu::find_compatible_format_for(vk::SurfaceKHR surf) const {
  const auto formats = phydev_.getSurfaceFormatsKHR(surf);
  const auto fmt_it = std::ranges::find_if(formats, [](vk::SurfaceFormatKHR fmt) {
    return fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
           (fmt.format == vk::Format::eR8G8B8A8Srgb || fmt.format == vk::Format::eB8G8R8A8Srgb);
  });
  if (fmt_it == formats.end())
    return std::nullopt;
  return fmt_it->format;
}

vk::SwapchainCreateInfoKHR
gpu::make_swapchain_info(vk::SurfaceKHR surf, vk::Format img_fmt, vk::Extent2D sz) const {
  auto res = make_swapchain_create_info(*phydev_, surf, img_fmt, sz);
  // TODO: Get rid of this strange patching.
  // FIXME: This code returns pointers to local array :( !!! And always was.
  std::array<uint32_t, 2> queues{families_.graphics, families_.presentation};
  if (families_.graphics != families_.presentation) {
    res.imageSharingMode = vk::SharingMode::eConcurrent;
    res.queueFamilyIndexCount = queues.size();
    res.pQueueFamilyIndices = queues.data();
  } else {
    res.queueFamilyIndexCount = 1;
    res.pQueueFamilyIndices = queues.data();
  }
  return res;
}

vlk::gpu select_suitable_device(vk::raii::Instance inst, vk::SurfaceKHR surf, vk::Extent2D sz) {
  for (vk::raii::PhysicalDevice dev : inst.enumeratePhysicalDevices()) {
    if (check_device(dev) && check_suitable_presentation_params(dev, surf)) {
      if (const auto families = vlk::device_queue_families::find(dev, surf)) {
        spdlog::info("Using Vulkan device: {}", std::string_view{dev.getProperties().deviceName});
        return vlk::gpu{std::move(inst), std::move(dev), *families};
      }
    }
    spdlog::debug("Vulkan device '{}' is rejected", std::string_view{dev.getProperties().deviceName});
  }

  throw std::runtime_error{"No suitable Vulkan device found"};
}

} // namespace vlk
