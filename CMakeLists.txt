cmake_minimum_required(VERSION 3.3)
project(wayland-test)

if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  set(CONAN_CMAKE_FIND_ROOT_PATH ${CONAN_CMAKE_MODULE_PATH})
  conan_basic_setup(TARGETS)
endif()

set(CMAKE_CXX_STANDARD 17)
set_directory_properties(PROPERTIES COMPILE_OPTIONS -Werror=all)

find_package(PkgConfig)

# wayland related deps
pkg_check_modules(wayland-client REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(wayland-egl REQUIRED IMPORTED_TARGET wayland-egl)

pkg_get_variable(WAYLAND_PROTOCOLS wayland-protocols pkgdatadir)
pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
  COMMAND ${WAYLAND_SCANNER} client-header ${WAYLAND_PROTOCOLS}/unstable/xdg-shell/xdg-shell-unstable-v6.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
  COMMAND ${WAYLAND_SCANNER} private-code ${WAYLAND_PROTOCOLS}/unstable/xdg-shell/xdg-shell-unstable-v6.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.c
  DEPENDS ${WAYLAND_PROTOCOLS}/unstable/xdg-shell/xdg-shell-unstable-v6.xml
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.c PROPERTIES GENERATED On)

# EGL GLES related deps
pkg_check_modules(egl REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(glesv2 REQUIRED IMPORTED_TARGET glesv2)
find_package(GLM)

add_library(GLM::glm IMPORTED INTERFACE)
set_target_properties(GLM::glm PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${GLM_INCLUDE_DIR}
)

add_executable(wlgles
  bitmask.hpp
  client.hpp
  conanfile.txt
  error.hpp
  file.hpp
  main.cpp
  shmem.hpp
  util.hpp
  wlutil.hpp
  xdg.hpp

  ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
  ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.c
)
target_link_libraries(wlgles
  PkgConfig::wayland-client PkgConfig::wayland-egl
  PkgConfig::egl PkgConfig::glesv2
  GLM::glm
  $<$<CXX_COMPILER_ID:Clang>:c++experimental>
  $<$<CXX_COMPILER_ID:GNU>:stdc++fs>
  CONAN_PKG::gsl_microsoft
)
target_include_directories(wlgles PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
