find_package(VulkanHeaders REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

cpp_unit(
  NAME vlk
  STD cxx_std_23
  LIBS
    vulkan-headers::vulkan-headers
    memtricks
  TEST_LIBS
    fmt::fmt
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
target_compile_definitions(vlk PUBLIC
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VK_USE_PLATFORM_WAYLAND_KHR
)
