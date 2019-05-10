cmake_minimum_required(VERSION 3.13)
project(wayland)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/conan.cmake)
conan_cmake_run(CONANFILE conanfile.txt
  BASIC_SETUP CMAKE_TARGETS
  BUILD missing
  SETTINGS arch=${CMAKE_SYSTEM_PROCESSOR}
  ENV CC=${CMAKE_C_COMPILER}
  ENV CXX=${CMAKE_CXX_COMPILER}
)

enable_testing()

option(WAYLAND_TEST_WERROR OFF "Treat warnings as errors")

find_package(Wayland REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS EGL OpenGL)

# wayland related deps
find_package(PkgConfig)
pkg_get_variable(WAYLAND_PROTOCOLS wayland-protocols pkgdatadir)

add_subdirectory(catch)
include(catch/add_catch_tests.cmake)
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
