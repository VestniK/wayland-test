find_package(PkgConfig)
pkg_check_modules(egl REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(gl REQUIRED IMPORTED_TARGET gl)

add_executable(gles
  main.cpp
)
target_link_libraries(gles wlpp-egl PkgConfig::egl PkgConfig::gl)
