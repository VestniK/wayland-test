find_package(VulkanHeaders REQUIRED)
find_package(Catch2 REQUIRED)
find_package(spdlog REQUIRED)

cpp_unit(
  NAME vk
  STD cxx_std_23
  LIBS
    vulkan-headers::vulkan-headers
    spdlog::spdlog
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
target_compile_definitions(vk PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
