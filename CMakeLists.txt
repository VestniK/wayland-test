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
include(cmake/zip_sfx.cmake)
include(FetchContent)
FetchContent_Declare(
  thinsys
  GIT_REPOSITORY https://github.com/VestniK/thinsys.git
  GIT_TAG f988028d5dada131388d57e8e4e42658fd0aa55e
  OVERRIDE_FIND_PACKAGE
)

add_subdirectory(tools)
add_subdirectory(testing)
add_subdirectory(libs)
add_subdirectory(apps)
