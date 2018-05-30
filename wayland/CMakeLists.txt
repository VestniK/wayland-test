find_package(PkgConfig)
pkg_check_modules(wayland-client REQUIRED IMPORTED_TARGET wayland-client)

add_headeronly_lib(wlpp
  client.hpp
  wl/basic_resource.hpp
  wl/buffer.hpp
  wl/callback.hpp
  wl/error.hpp
  wl/io.hpp
  wl/pointer.hpp
  wl/keyboard.hpp
  wl/registry.hpp
  wl/seat.hpp
  wl/shell_surface.hpp
  wl/shm.hpp
  wl/shm_pool.hpp
  wl/surface.hpp
  wl/util.hpp
)
target_include_directories(wlpp INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(wlpp INTERFACE util io PkgConfig::wayland-client CONAN_PKG::gsl_microsoft)
