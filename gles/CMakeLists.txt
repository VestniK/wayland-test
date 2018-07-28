find_package(PkgConfig)
pkg_check_modules(egl REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(glesv2 REQUIRED IMPORTED_TARGET glesv2)
find_package(GLM)

pkg_get_variable(WAYLAND_PROTOCOLS wayland-protocols pkgdatadir)
pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
  COMMAND ${WAYLAND_SCANNER} client-header ${WAYLAND_PROTOCOLS}/unstable/xdg-shell/xdg-shell-unstable-v6.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
  DEPENDS ${WAYLAND_PROTOCOLS}/unstable/xdg-shell/xdg-shell-unstable-v6.xml
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h PROPERTIES GENERATED On)

add_library(GLM::glm IMPORTED INTERFACE)
set_target_properties(GLM::glm PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${GLM_INCLUDE_DIR}
)

add_executable(gles
  main.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/xdg_shell.h
)
target_link_libraries(gles wlpp PkgConfig::egl PkgConfig::glesv2 GLM::glm)
target_include_directories(gles PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
