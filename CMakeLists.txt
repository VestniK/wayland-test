cmake_minimum_required(VERSION 3.3)
project(wayland-test)

if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  set(CONAN_CMAKE_FIND_ROOT_PATH ${CONAN_CMAKE_MODULE_PATH})
  conan_basic_setup(TARGETS)
endif()

set(CMAKE_CXX_STANDARD 17)
set_directory_properties(PROPERTIES COMPILE_OPTIONS -Werror=all)

find_package(log4cplus REQUIRED)
find_package(fmt REQUIRED)
find_package(portable_concurrency REQUIRED)

function(add_headeronly_lib tgt)
  add_library(${tgt} INTERFACE)
  add_library(${tgt}-ide OBJECT ${ARGN})
  set_target_properties(${tgt}-ide PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

add_library(xkb::common INTERFACE IMPORTED)
set_target_properties(xkb::common PROPERTIES INTERFACE_LINK_LIBRARIES xkbcommon)

add_subdirectory(util)
add_subdirectory(io)
add_subdirectory(wayland)

add_executable(lswl lswl.cpp)
target_link_libraries(lswl PRIVATE wlpp)

add_executable(wayland_test
  xkb.hpp
  registry_searcher.hpp
  main.cpp
  conanfile.txt
)
target_link_libraries(wayland_test
  portable_concurrency::portable_concurrency
  log4cplus::log4cplus
  fmt::fmt
  xkb::common
  io
  wlpp
)
target_compile_definitions(wayland_test PRIVATE $<$<NOT:$<CONFIG:Debug>>:LOG4CPLUS_DISABLE_DEBUG>)
