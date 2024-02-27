#include <vk/prepare_instance.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <spdlog/spdlog.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void prepare_instance() {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  vk::raii::Context context;

  vk::ApplicationInfo app_info(
      "wayland-test", 1, "no_engine", 1, VK_API_VERSION_1_0);
  vk::InstanceCreateInfo inst_create_info({}, &app_info);
  vk::raii::Instance inst{context, inst_create_info, nullptr};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*inst);

  for (const vk::raii::PhysicalDevice& dev : inst.enumeratePhysicalDevices()) {
    const auto props = dev.getProperties();
    spdlog::debug(
        "Found Vulkan device: {}", std::string_view{props.deviceName});
  }
}
