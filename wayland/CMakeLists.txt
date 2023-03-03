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
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ivi-application.h
  COMMAND Wayland::scanner client-header ${LOCAL_WAYLAND_PROTOCOLS}/ivi-application.xml ${CMAKE_CURRENT_BINARY_DIR}/ivi-application.h
  COMMAND Wayland::scanner private-code ${LOCAL_WAYLAND_PROTOCOLS}/ivi-application.xml ${CMAKE_CURRENT_BINARY_DIR}/ivi-application.c
  DEPENDS ${LOCAL_WAYLAND_PROTOCOLS}/ivi-application.xml
)
set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/ivi-application.h
  ${CMAKE_CURRENT_BINARY_DIR}/ivi-application.c
  PROPERTIES
    GENERATED ON
)

add_library(wayland.obj OBJECT
  egl.cpp
  egl.hpp
  event_loop.cpp
  event_loop.hpp
  ui_category.cpp
  ui_category.hpp
  wayland-client.hpp
  ivi_window.cpp
  ivi_window.hpp
  vsync_frames.cpp
  vsync_frames.hpp
  xdg_window.cpp
  xdg_window.hpp
  window_delegate.hpp
  wlutil.hpp
  gles_window.cpp
  gles_window.hpp

  ${CMAKE_CURRENT_BINARY_DIR}/ivi-application.h
  ${CMAKE_CURRENT_BINARY_DIR}/ivi-application.c
  ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h
  ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c
)
target_compile_features(wayland.obj PUBLIC cxx_std_20)
target_link_libraries(wayland.obj
  util
  gles2
  Wayland::client Wayland::egl
  OpenGL::EGL
  asio::asio
  spdlog::spdlog
  ${CMAKE_DL_LIBS}
)
target_include_directories(wayland.obj PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${wayland_SOURCE_DIR})
target_compile_definitions(wayland.obj PUBLIC
    # FIXME: For QtCreator clang based code completion only
    ASIO_HAS_CO_AWAIT
    ASIO_HAS_STD_COROUTINE
)
