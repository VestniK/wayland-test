find_package(VulkanHeaders REQUIRED COMPONENTS Headers)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(Catch2 REQUIRED)
find_package(spdlog REQUIRED)

cpp_unit(
  NAME vlk
  STD cxx_std_23
  LIBS
    Vulkan::Headers
    GPUOpen::VulkanMemoryAllocator
    spdlog::spdlog
    memtricks
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
target_compile_definitions(vlk PUBLIC
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VK_USE_PLATFORM_WAYLAND_KHR
)

target_sources(vlk PUBLIC
  FILE_SET CXX_MODULES
  FILES
    buf.cppm
    cmds.cppm
    gpu.cppm
    pipelines.cppm
    presentation.cppm
    resource_uploader.cppm
    uniforms.cppm
    vertex.cppm
    vma.cppm
)
