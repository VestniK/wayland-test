cmake_minimum_required(VERSION 3.8)
project(wayland-test)

if (POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  set(CONAN_CMAKE_FIND_ROOT_PATH ${CONAN_CMAKE_MODULE_PATH})
  conan_basic_setup(TARGETS)
endif()

set(CMAKE_CXX_STANDARD 17)
set_directory_properties(PROPERTIES COMPILE_OPTIONS -Werror=all)

find_package(Wayland REQUIRED)
find_package(GLM REQUIRED)

find_package(PkgConfig)

# wayland related deps
pkg_check_modules(wayland-egl REQUIRED IMPORTED_TARGET wayland-egl)

pkg_get_variable(WAYLAND_PROTOCOLS wayland-protocols pkgdatadir)
pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
  COMMAND ${WAYLAND_SCANNER} client-header ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
  COMMAND ${WAYLAND_SCANNER} private-code ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.c
  DEPENDS ${WAYLAND_PROTOCOLS}/unstable/xdg-shell/xdg-shell-unstable-v6.xml
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.c PROPERTIES GENERATED On)

# EGL GLES related deps
pkg_check_modules(egl REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(glesv2 REQUIRED IMPORTED_TARGET glesv2)

add_library(GLM::glm IMPORTED INTERFACE)
set_target_properties(GLM::glm PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${GLM_INCLUDE_DIR}
)

add_executable(wlgles
  bitmask.hpp
  client.hpp
  conanfile.txt
  egl.h
  egl.cpp
  error.hpp
  file.hpp
  geom.h
  main.cpp
  renderer.h
  renderer.cpp
  shmem.hpp
  util.hpp
  window.h
  window.cpp
  wlutil.hpp
  xdg.hpp

  ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
  ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.c
)
target_link_libraries(wlgles
  Wayland::client
  PkgConfig::wayland-egl
  PkgConfig::egl PkgConfig::glesv2
  GLM::glm
  $<$<CXX_COMPILER_ID:Clang>:c++experimental>
  $<$<CXX_COMPILER_ID:GNU>:stdc++fs>
  CONAN_PKG::gsl_microsoft
)
target_include_directories(wlgles PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
