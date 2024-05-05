find_package(VulkanHeaders REQUIRED)
find_package(Catch2 REQUIRED)
find_package(spdlog REQUIRED)

cpp_unit(
  NAME vk
  STD cxx_std_23
  LIBS
    anime
    geom
    vulkan-headers::vulkan-headers
    spdlog::spdlog
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
target_compile_definitions(vk PRIVATE
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VULKAN_HPP_NO_CONSTRUCTORS
  VK_USE_PLATFORM_WAYLAND_KHR
)

find_program(GLSLC NAMES glslc REQUIRED)
find_program(LD NAMES ld lld REQUIRED)
find_program(OBJCOPY NAMES objcopy llvm-objcopy REQUIRED)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/triangle.frag.o
  COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag -o ${CMAKE_CURRENT_BINARY_DIR}/triangle.frag.spv
  COMMAND ${LD} --format=binary -z noexecstack -r -o triangle.frag.o triangle.frag.spv
  COMMAND ${OBJCOPY} --rename-section .data=.rodata,alloc,load,readonly,data,contents triangle.frag.o
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/triangle.vert.o
  COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert -o ${CMAKE_CURRENT_BINARY_DIR}/triangle.vert.spv
  COMMAND ${LD} --format=binary -z noexecstack -r -o triangle.vert.o triangle.vert.spv
  COMMAND ${OBJCOPY} --rename-section .data=.rodata,alloc,load,readonly,data,contents triangle.vert.o
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
target_sources(vk PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/triangle.frag.o ${CMAKE_CURRENT_BINARY_DIR}/triangle.vert.o)
