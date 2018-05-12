cmake_minimum_required(VERSION 3.3)
project(wayland-test)

set(CMAKE_CXX_STANDARD 17)
set_directory_properties(PROPERTIES COMPILE_OPTIONS -Werror=all)

find_package(portable_concurrency REQUIRED)

add_library(wayland::client INTERFACE IMPORTED)
set_target_properties(wayland::client PROPERTIES INTERFACE_LINK_LIBRARIES wayland-client)

add_library(wlpp-ide OBJECT
  wayland/client.hpp
  wayland/wl/basic_listener.hpp
  wayland/wl/basic_resource.hpp
  wayland/wl/bitmask.hpp
  wayland/wl/buffer.hpp
  wayland/wl/callback.hpp
  wayland/wl/error.hpp
  wayland/wl/keyboard.hpp
  wayland/wl/registry.hpp
  wayland/wl/seat.hpp
  wayland/wl/shell_surface.hpp
  wayland/wl/shm.hpp
  wayland/wl/shm_pool.hpp
  wayland/wl/surface.hpp
  wayland/wl/util.hpp
)
set_target_properties(wlpp-ide PROPERTIES LINKER_LANGUAGE CXX)
add_library(wlpp INTERFACE)
target_include_directories(wlpp INTERFACE
  $<BUILD_INTERFACE:${CURRENT_SOURCE_DIRECTORY}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(wlpp INTERFACE wayland::client)

add_executable(lswl
  lswl.cpp
)
target_link_libraries(lswl PRIVATE wlpp)

add_executable(wayland_test
  io.h
  io.cpp
  xdg.h
  xdg.cpp
  registry_searcher.hpp
  main.cpp
)
target_link_libraries(wayland_test
  portable_concurrency::portable_concurrency
  wlpp
)
target_compile_options(wayland_test PRIVATE
  -fcoroutines-ts
)
