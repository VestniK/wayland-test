cmake_minimum_required(VERSION 3.25)
project(wayland)

set(CMAKE_CXX_EXTENSIONS Off)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

option(CMAKE_COMPILE_WARNING_AS_ERROR OFF "Treat warnings as errors")
option(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE On "Enable LTO in release builds")

find_package(Wayland REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS EGL OpenGL)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(asio REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)

# wayland related deps
find_package(PkgConfig)
pkg_get_variable(WAYLAND_PROTOCOLS wayland-protocols pkgdatadir)
set(LOCAL_WAYLAND_PROTOCOLS ${CMAKE_CURRENT_SOURCE_DIR}/protocols)

pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

include(cmake/canonical_project.cmake)

add_subdirectory(corort)
add_subdirectory(util)
add_subdirectory(gamepad)
add_subdirectory(gles2)
add_subdirectory(wayland)

add_executable(wayland
  conanfile.txt
  .clang-format
  wayland/main.cpp
)
target_link_libraries(wayland PRIVATE
  spdlog::spdlog
  PkgConfig::SYSTEMD
  wayland.obj
  util
  gles2
  corort
  gamepad
  fmt::fmt
)
add_custom_command(TARGET wayland POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:wayland> $<TARGET_FILE:wayland>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:wayland>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:wayland>.dbg $<TARGET_FILE:wayland>
)
