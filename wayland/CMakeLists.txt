find_package(PkgConfig)
pkg_check_modules(wayland-client REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(wayland-egl REQUIRED IMPORTED_TARGET wayland-egl)

add_headeronly_lib(wlpp
  client.hpp
  wl/basic_resource.hpp
  wl/buffer.hpp
  wl/callback.hpp
  wl/error.hpp
  wl/event_queue.hpp
  wl/io.hpp
  wl/pointer.hpp
  wl/keyboard.hpp
  wl/registry.hpp
  wl/seat.hpp
  wl/shell_surface.hpp
  wl/shm.hpp
  wl/shm_pool.hpp
  wl/subcompositor.hpp
  wl/subsurface.hpp
  wl/surface.hpp
  wl/util.hpp
)
target_include_directories(wlpp INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(wlpp INTERFACE
  util io
  PkgConfig::wayland-client
  CONAN_PKG::gsl_microsoft
)

add_headeronly_lib(wlpp-egl
  egl.hpp
)
target_include_directories(wlpp-egl INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(wlpp-egl INTERFACE
  wlpp PkgConfig::wayland-egl
)
