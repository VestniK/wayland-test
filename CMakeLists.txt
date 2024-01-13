cmake_minimum_required(VERSION 3.27)
project(wayland-test)

set(CMAKE_CXX_EXTENSIONS Off)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

add_compile_options(-Wall)
option(CMAKE_COMPILE_WARNING_AS_ERROR OFF "Treat warnings as errors")
option(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE On "Enable LTO in release builds")

include(cmake/canonical_project.cmake)
include(cmake/sanitizers.cmake)
include(cmake/wayland_protocols.cmake)
include(FetchContent)
FetchContent_Declare(
  thinsys
  GIT_REPOSITORY https://github.com/VestniK/thinsys.git
  GIT_TAG 962daa53b8ca70b2fbe9908bbf22348f0fa975ec
  OVERRIDE_FIND_PACKAGE
)

add_subdirectory(testing)
add_subdirectory(corort)
add_subdirectory(util)
add_subdirectory(gamepad)
add_subdirectory(gles2)
add_subdirectory(img)
add_subdirectory(scene)
add_subdirectory(wayland)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# wayland related deps
find_package(PkgConfig)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

add_executable(wayland-test
  conanfile.txt
  .clang-format
  .clangd
  main.cpp
  cmake/sanitizers.cmake
)
target_link_libraries(wayland-test PRIVATE
  spdlog::spdlog
  PkgConfig::SYSTEMD
  wlwnd
  util
  gles2
  scene
  corort
  gamepad
  fmt::fmt
)
add_custom_command(TARGET wayland-test POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:wayland-test> $<TARGET_FILE:wayland-test>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:wayland-test>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:wayland-test>.dbg $<TARGET_FILE:wayland-test>
)
