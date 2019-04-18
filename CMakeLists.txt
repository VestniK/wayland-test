cmake_minimum_required(VERSION 3.13)
project(wayland)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
set(CONAN_CMAKE_FIND_ROOT_PATH ${CONAN_CMAKE_MODULE_PATH})
conan_basic_setup(TARGETS)

enable_testing()

option(WAYLAND_TEST_WERROR OFF "Treat warnings as errors")

find_package(Wayland REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS EGL OpenGL)

# wayland related deps
find_package(PkgConfig)
pkg_get_variable(WAYLAND_PROTOCOLS wayland-protocols pkgdatadir)

add_subdirectory(wayland)

add_executable(wayland
  conanfile.txt
  .clang-format
  scripts/wayland.chai
  wayland/main.cpp
)
target_link_libraries(wayland PRIVATE wayland.obj util gles2)
add_custom_command(TARGET wayland POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:wayland> $<TARGET_FILE:wayland>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:wayland>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:wayland>.dbg $<TARGET_FILE:wayland>
)
