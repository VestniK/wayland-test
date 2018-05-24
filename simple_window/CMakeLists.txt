find_package(log4cplus REQUIRED)
find_package(fmt REQUIRED)
find_package(portable_concurrency REQUIRED)

find_package(PkgConfig)
pkg_check_modules(cairomm REQUIRED IMPORTED_TARGET cairomm-1.0)
pkg_check_modules(xkbcommon REQUIRED IMPORTED_TARGET xkbcommon)

add_executable(simple_window
  main.cpp
  quit_waiter.hpp
  registry_searcher.hpp
  xkb.hpp
)
target_link_libraries(simple_window
  portable_concurrency::portable_concurrency
  log4cplus::log4cplus
  fmt::fmt
  PkgConfig::xkbcommon
  PkgConfig::cairomm
  io
  wlpp
)
target_compile_definitions(simple_window PRIVATE $<$<NOT:$<CONFIG:Debug>>:LOG4CPLUS_DISABLE_DEBUG>)
