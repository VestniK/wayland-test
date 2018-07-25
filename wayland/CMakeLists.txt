find_package(PkgConfig)
pkg_check_modules(wayland-client REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(wayland-egl REQUIRED IMPORTED_TARGET wayland-egl)

add_headeronly_lib(wlpp
  client.hpp
  wl/error.hpp
  wl/util.hpp
)
target_include_directories(wlpp INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(wlpp INTERFACE
  util io
  PkgConfig::wayland-client
  PkgConfig::wayland-egl
  CONAN_PKG::gsl_microsoft
)
