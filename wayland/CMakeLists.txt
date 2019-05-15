add_subdirectory(util)
add_subdirectory(gles2)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h
  COMMAND Wayland::scanner client-header ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h
  COMMAND Wayland::scanner private-code ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c
  DEPENDS ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml
)
set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h
  ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c
  PROPERTIES
    GENERATED ON
)

add_library(wayland.obj OBJECT
  egl.cpp
  egl.hpp
  event_loop.cpp
  event_loop.hpp
  script_player.cpp
  script_player.hpp
  ui_category.cpp
  ui_category.hpp
  wayland-client.hpp
  window.cpp
  window.hpp
  wlutil.hpp
  gles_window.cpp
  gles_window.hpp

  ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h
  ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c
)
target_compile_features(wayland.obj PUBLIC cxx_std_17)
if (WAYLAND_TEST_WERROR)
  target_compile_options(wayland.obj PUBLIC -Werror=all)
endif()
target_link_libraries(wayland.obj
  util
  gles2
  stdc++fs
  Wayland::client Wayland::egl
  OpenGL::EGL
  CONAN_PKG::gsl_microsoft
  CONAN_PKG::chaiscript ${CMAKE_DL_LIBS}
)
target_include_directories(wayland.obj PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${wayland_SOURCE_DIR})
target_compile_definitions(wayland.obj PUBLIC CHAISCRIPT_NO_THREADS)
