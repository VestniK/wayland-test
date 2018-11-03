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

# wayland-scanner
pkg_check_modules(PC_WAYLAND_SCANNER QUIET wayland-scanner)
find_program(WAYLAND_SCANNER
  NAMES wayland-scanner
  PATHS ${PC_WAYLAND_SCANNER_PREFIX}
)
mark_as_advanced(WAYLAND_SCANNER)

# handle sandard args
set(WAYLAND_VERSION ${PC_WAYLAND_CLIENT_VERSION})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wayland
  REQUIRED_VARS
    WAYLAND_CLIENT_INCLUDE_DIR WAYLAND_CLIENT_LIBRARY
    WAYLAND_EGL_INCLUDE_DIR WAYLAND_EGL_LIBRARY
    WAYLAND_SCANNER
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

if (NOT TARGET Wayland::scanner)
  add_executable(Wayland::scanner IMPORTED)
  set_target_properties(Wayland::scanner PROPERTIES
    IMPORTED_LOCATION "${WAYLAND_SCANNER}"
  )
endif()
