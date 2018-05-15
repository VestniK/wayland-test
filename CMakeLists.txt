cmake_minimum_required(VERSION 3.3)
project(wayland-test)

set(CMAKE_CXX_STANDARD 17)
set_directory_properties(PROPERTIES COMPILE_OPTIONS -Werror=all)

function(add_headeronly_lib tgt)
  add_library(${tgt} INTERFACE)
  add_library(${tgt}-ide ${ARGN})
  set_target_properties(${tgt}-ide PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(${tgt}-ide ${tgt})
endfunction()

find_package(portable_concurrency REQUIRED)

add_library(wayland::client INTERFACE IMPORTED)
set_target_properties(wayland::client PROPERTIES INTERFACE_LINK_LIBRARIES wayland-client)

add_library(xkb::common INTERFACE IMPORTED)
set_target_properties(xkb::common PROPERTIES INTERFACE_LINK_LIBRARIES xkbcommon)

add_headeronly_lib(util
  util/bitmask.hpp
  util/util.hpp
)
target_include_directories(util INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

add_headeronly_lib(io
  io/file.hpp
  io/shmem.hpp
  io/xdg.hpp
)
target_include_directories(io INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(io INTERFACE util)

add_headeronly_lib(wlpp
  wayland/client.hpp
  wayland/wl/basic_listener.hpp
  wayland/wl/basic_resource.hpp
  wayland/wl/buffer.hpp
  wayland/wl/callback.hpp
  wayland/wl/error.hpp
  wayland/wl/io.hpp
  wayland/wl/keyboard.hpp
  wayland/wl/registry.hpp
  wayland/wl/seat.hpp
  wayland/wl/shell_surface.hpp
  wayland/wl/shm.hpp
  wayland/wl/shm_pool.hpp
  wayland/wl/surface.hpp
  wayland/wl/util.hpp
)
target_include_directories(wlpp INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(wlpp INTERFACE util io wayland::client)

add_executable(lswl lswl.cpp)
target_link_libraries(lswl PRIVATE wlpp)

add_executable(wayland_test
  xkb.hpp
  registry_searcher.hpp
  main.cpp
)
target_link_libraries(wayland_test
  portable_concurrency::portable_concurrency
  xkb::common
  io
  wlpp
)
target_compile_options(wayland_test PRIVATE
  -fcoroutines-ts
)
