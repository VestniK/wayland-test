#include <vk/prepare_instance.hpp>

#include <algorithm>
#include <optional>
#include <ranges>
#include <tuple>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <spdlog/spdlog.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {

std::array<const char*, 2> REQUIRED_EXTENSIONS{
    "VK_KHR_surface", "VK_KHR_wayland_surface"};

std::array<const char*, 1> REQUIRED_DEVICE_EXTENSIONS{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

vk::raii::Instance create_instance() {
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
  const auto inst_create_info =
      vk::InstanceCreateInfo{.flags = {}, .pApplicationInfo = &app_info}
          .setPEnabledExtensionNames(REQUIRED_EXTENSIONS);
  vk::raii::Instance inst{context, inst_create_info, nullptr};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*inst);
  return inst;
}

vk::raii::Device create_logical_device(const vk::raii::PhysicalDevice& dev,
    uint32_t graphics_queue_family, uint32_t presentation_queue_family) {
  float queue_prio = 1.0f;
  std::array<vk::DeviceQueueCreateInfo, 2> device_queues{
      vk::DeviceQueueCreateInfo{.flags = {},
          .queueFamilyIndex = graphics_queue_family,
          .queueCount = 1,
          .pQueuePriorities = &queue_prio},
      vk::DeviceQueueCreateInfo{.flags = {},
          .queueFamilyIndex = presentation_queue_family,
          .queueCount = 1,
          .pQueuePriorities = &queue_prio}};
  vk::DeviceCreateInfo device_create_info{.flags = {},
      .queueCreateInfoCount = graphics_queue_family == presentation_queue_family
                                  ? 1u
                                  : 2u, // TODO: better duplication needed
      .pQueueCreateInfos = device_queues.data(),
      .enabledExtensionCount = REQUIRED_DEVICE_EXTENSIONS.size(),
      .ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data()};

  vk::raii::Device device{dev, device_create_info};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
  return device;
}

class render_environment {
public:
  render_environment(const vk::raii::PhysicalDevice& dev,
      uint32_t graphics_queue_family, uint32_t presentation_queue_family)
      : device{create_logical_device(
            dev, graphics_queue_family, presentation_queue_family)},
        graphics_queue{device.getQueue(graphics_queue_family, 0)},
        presentation_queue{device.getQueue(presentation_queue_family, 0)} {}

private:
  vk::raii::Device device;
  vk::raii::Queue graphics_queue;
  vk::raii::Queue presentation_queue;
};

render_environment setup_suitable_device(
    const vk::raii::Instance& inst, const vk::SurfaceKHR& surf) {
  auto devices = inst.enumeratePhysicalDevices();

  for (const vk::raii::PhysicalDevice& dev : inst.enumeratePhysicalDevices()) {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> presentation_family;
    for (const auto& [idx, queue_family] :
        std::views::enumerate(dev.getQueueFamilyProperties())) {
      if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
        graphics_family = idx;
      if (dev.getSurfaceSupportKHR(idx, surf))
        presentation_family = idx;
    }

    // TODO: send supported dev extensions to debug log
    const bool has_swapchain_extension = std::ranges::any_of(
        dev.enumerateDeviceExtensionProperties(),
        [](vk::ExtensionProperties prop) {
          return std::string_view{prop.extensionName} ==
                 VK_KHR_SWAPCHAIN_EXTENSION_NAME; // TODO: use
                                                  // REQUIRED_DEVICE_EXTENSIONS
                                                  // instead of checking ext
                                                  // directly
        });

    if (graphics_family && presentation_family && has_swapchain_extension) {
      spdlog::info("Using Vulkan device: {}",
          std::string_view{dev.getProperties().deviceName});
      return {dev, *graphics_family, *presentation_family};
    }

    spdlog::debug("Vulkan device '{}' is rejected",
        std::string_view{dev.getProperties().deviceName});
  }

  throw std::runtime_error{"No suitable Vulkan device found"};
}

} // namespace

void prepare_instance(wl_display& display, wl_surface& surf) {
  vk::raii::Instance inst = create_instance();
  vk::raii::SurfaceKHR vk_surf{
      inst, vk::WaylandSurfaceCreateInfoKHR{
                .flags = {}, .display = &display, .surface = &surf}};

  render_environment render_env = setup_suitable_device(inst, *vk_surf);
}
