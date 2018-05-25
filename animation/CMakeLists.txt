find_package(PkgConfig)
pkg_check_modules(cairomm REQUIRED IMPORTED_TARGET cairomm-1.0)

add_executable(animation main.cpp)
target_link_libraries(animation PRIVATE
  wlpp
  io
  CONAN_PKG::gsl_microsoft
  PkgConfig::cairomm
)
