find_package(PkgConfig)
find_package(Threads)
pkg_check_modules(cairomm REQUIRED IMPORTED_TARGET cairomm-1.0)

add_executable(concurrent main.cpp)
target_link_libraries(concurrent PRIVATE
  wlpp
  io
  CONAN_PKG::gsl_microsoft
  PkgConfig::cairomm
  Threads::Threads
)
