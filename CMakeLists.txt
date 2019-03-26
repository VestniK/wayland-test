cmake_minimum_required(VERSION 3.10)
project(wayland)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
set(CONAN_CMAKE_FIND_ROOT_PATH ${CONAN_CMAKE_MODULE_PATH})
conan_basic_setup(TARGETS)

option(WAYLAND_TEST_WERROR OFF "Treat warnings as errors")

find_package(Wayland REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS EGL OpenGL)

# wayland related deps
find_package(PkgConfig)
pkg_get_variable(WAYLAND_PROTOCOLS wayland-protocols pkgdatadir)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h
  COMMAND Wayland::scanner client-header ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h
  COMMAND Wayland::scanner code ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c
  DEPENDS ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c PROPERTIES GENERATED ON)

add_executable(wayland
  conanfile.txt
  scripts/wayland.chai

  wayland/egl.cpp
  wayland/egl.hpp
  wayland/event_loop.cpp
  wayland/event_loop.hpp
  wayland/geom.hpp
  wayland/get_option.cpp
  wayland/get_option.hpp
  wayland/main.cpp
  wayland/script_player.cpp
  wayland/script_player.hpp
  wayland/ui_category.cpp
  wayland/ui_category.hpp
  wayland/renderer.cpp
  wayland/renderer.hpp
  wayland/wayland-client.hpp
  wayland/window.cpp
  wayland/window.hpp
  wayland/wlutil.hpp
  wayland/xdg.cpp
  wayland/xdg.hpp
  wayland/gles_window.cpp
  wayland/gles_window.hpp

  ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h
  ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c
)
target_compile_features(wayland PRIVATE cxx_std_17)
if (WAYLAND_TEST_WERROR)
  target_compile_options(wayland PRIVATE -Werror=all)
endif()
target_link_libraries(wayland
  stdc++fs
  Wayland::client Wayland::egl
  OpenGL::OpenGL OpenGL::EGL
  CONAN_PKG::glm
  CONAN_PKG::gsl_microsoft
  CONAN_PKG::chaiscript ${CMAKE_DL_LIBS}
)
target_include_directories(wayland PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${wayland_SOURCE_DIR})
target_compile_definitions(wayland PRIVATE CHAISCRIPT_NO_THREADS)

add_custom_command(TARGET wayland POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:wayland> $<TARGET_FILE:wayland>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:wayland>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:wayland>.dbg $<TARGET_FILE:wayland>
)
