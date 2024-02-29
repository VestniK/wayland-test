#include <vk/prepare_instance.hpp>

#include <algorithm>
#include <ranges>
#include <tuple>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <spdlog/spdlog.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {

template <std::ranges::forward_range Rng, typename Pred>
  requires std::convertible_to<
      std::invoke_result_t<Pred, std::ranges::range_reference_t<Rng>>, bool>
std::ranges::range_difference_t<Rng> index_of(Rng&& rng, Pred&& pred) {
  const auto it = std::ranges::find_if(rng, std::forward<Pred>(pred));
  return it == std::ranges::end(rng)
             ? -1
             : std::distance(std::ranges::begin(rng), it);
}

std::tuple<vk::raii::PhysicalDevice, uint32_t> find_suitable_device(
    const vk::raii::Instance& inst) {
  auto devices = inst.enumeratePhysicalDevices();

  uint32_t queue_idx = 0;
  const auto it = std::ranges::find_if(
      devices, [&queue_idx](const vk::raii::PhysicalDevice& dev) {
        const std::ptrdiff_t idx =
            index_of(dev.getQueueFamilyProperties(), [](const auto& prop) {
              return static_cast<bool>(
                  prop.queueFlags & vk::QueueFlagBits::eGraphics);
            });
        if (idx < 0)
          return false;
        queue_idx = static_cast<uint32_t>(idx);
        return true;
      });
  if (it == devices.end()) {
    throw std::runtime_error{"No graphics capable hardware found"};
  }
  return {std::move(*it), queue_idx};
}

} // namespace

void prepare_instance(wl_display& display, wl_surface& surf) {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  for (const auto& ext : vk::enumerateInstanceExtensionProperties()) {
    spdlog::debug("Vulkan supported extension: {} [ver={}]",
        std::string_view{ext.extensionName}, ext.specVersion);
  }

  vk::raii::Context context;

  vk::ApplicationInfo app_info{.pApplicationName = "wayland-test",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "no_engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0};
  std::array<const char*, 2> exts{"VK_KHR_surface", "VK_KHR_wayland_surface"};
  const auto inst_create_info =
      vk::InstanceCreateInfo{.flags = {}, .pApplicationInfo = &app_info}
          .setPEnabledExtensionNames(exts);
  vk::raii::Instance inst{context, inst_create_info, nullptr};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*inst);

  const auto [phis_dev, qraphics_queue_idx] = find_suitable_device(inst);
  const auto props = phis_dev.getProperties();
  spdlog::debug("Using Vulkan device: {}", std::string_view{props.deviceName});

  float queue_prio = 1.0f;
  vk::DeviceQueueCreateInfo dev_queue_create_info{.flags = {},
      .queueFamilyIndex = qraphics_queue_idx,
      .queueCount = 1,
      .pQueuePriorities = &queue_prio};
  vk::DeviceCreateInfo device_create_info{
      .flags = {}, .pQueueCreateInfos = &dev_queue_create_info};

  vk::raii::Device device{phis_dev, device_create_info};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
  vk::raii::Queue queue = device.getQueue(qraphics_queue_idx, 0);

  vk::WaylandSurfaceCreateInfoKHR surf_info{
      .flags = {}, .display = &display, .surface = &surf};
  vk::raii::SurfaceKHR vk_surf{inst, surf_info};
}
