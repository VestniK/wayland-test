find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(VulkanHeaders REQUIRED)

find_package(PkgConfig)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

cpp_unit(
  NAME castle
  STD cxx_std_23
  LIBS
    asio::asio
    cli
    corort
    fmt::fmt
    img
    PkgConfig::SYSTEMD
    sfx
    spdlog::spdlog
    vlk
    vulkan-headers::vulkan-headers
    wlwnd
    xdg
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
target_compile_definitions(castle.lib PRIVATE
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
target_sources(castle PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/triangle.frag.o
  ${CMAKE_CURRENT_BINARY_DIR}/triangle.vert.o
  shaders/triangle.frag
  shaders/triangle.vert
)


target_sources(castle PRIVATE
  ${CMAKE_SOURCE_DIR}/conanfile.py
  ${CMAKE_SOURCE_DIR}/.clang-format
)
add_custom_command(TARGET castle POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:castle> $<TARGET_FILE:castle>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:castle>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:castle>.dbg $<TARGET_FILE:castle>
)
zip_sfx(castle
  textures/castle.png
)

add_subdirectory(vlk)
