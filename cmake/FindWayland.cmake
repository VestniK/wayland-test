find_package(PkgConfig QUIET)

# wayland-client
pkg_check_modules(PC_WAYLAND_CLIENT QUIET wayland-client)
find_path(WAYLAND_CLIENT_INCLUDE_DIR
  NAMES wayland-client.h
  PATHS
    ${PC_WAYLAND_CLIENT_INCLUDE_DIRS}
  PATH_SUFFIXES wayland
)
find_library(WAYLAND_CLIENT_LIBRARY
  NAMES wayland-client
  PATHS ${PC_WAYLAND_CLIENT_LIBRARY_DIRS}
)
mark_as_advanced(
  WAYLAND_CLIENT_INCLUDE_DIR
  WAYLAND_CLIENT_LIBRARY
)

# wayland-egl
pkg_check_modules(PC_WAYLAND_EGL QUIET wayland-egl)
find_path(WAYLAND_EGL_INCLUDE_DIR
  NAMES wayland-egl.h
  PATHS
    ${PC_WAYLAND_EGL_INCLUDE_DIRS}
  PATH_SUFFIXES wayland
)
find_library(WAYLAND_EGL_LIBRARY
  NAMES wayland-egl
  PATHS ${PC_WAYLAND_EGL_LIBRARY_DIRS}
)
mark_as_advanced(
  WAYLAND_EGL_INCLUDE_DIR
  WAYLAND_EGL_LIBRARY
)

# handle sandard args
set(WAYLAND_VERSION ${PC_WAYLAND_CLIENT_VERSION})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wayland
  REQUIRED_VARS
    WAYLAND_CLIENT_INCLUDE_DIR WAYLAND_CLIENT_LIBRARY
    WAYLAND_EGL_INCLUDE_DIR WAYLAND_EGL_LIBRARY
  VERSION_VAR WAYLAND_VERSION
)

# imported targets
if (NOT WAYLAND_FOUND)
  return()
endif()

if (NOT TARGET Wayland::client)
  add_library(Wayland::client UNKNOWN IMPORTED)
  set_target_properties(Wayland::client PROPERTIES
    IMPORTED_LOCATION "${WAYLAND_CLIENT_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_CLIENT_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${PC_WAYLAND_CLIENT_CFLAGS_OTHER}"
  )
endif()

if (NOT TARGET Wayland::egl)
  add_library(Wayland::egl UNKNOWN IMPORTED)
  set_target_properties(Wayland::egl PROPERTIES
    IMPORTED_LOCATION "${WAYLAND_EGL_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_EGL_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${PC_WAYLAND_EGL_CFLAGS_OTHER}"
  )
endif()
