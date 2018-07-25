cmake_minimum_required(VERSION 3.3)
project(wayland-test)

if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  set(CONAN_CMAKE_FIND_ROOT_PATH ${CONAN_CMAKE_MODULE_PATH})
  conan_basic_setup(TARGETS)
endif()

set(CMAKE_CXX_STANDARD 17)
set_directory_properties(PROPERTIES COMPILE_OPTIONS -Werror=all)

function(add_headeronly_lib tgt)
  add_library(${tgt} INTERFACE)
  add_library(${tgt}-ide OBJECT ${ARGN})
  set_target_properties(${tgt}-ide PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

add_subdirectory(util)
add_subdirectory(io)
add_subdirectory(wayland)

add_subdirectory(gles)
