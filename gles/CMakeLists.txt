find_package(PkgConfig)
pkg_check_modules(egl REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(glesv2 REQUIRED IMPORTED_TARGET glesv2)
find_package(GLM)

add_library(GLM::glm IMPORTED INTERFACE)
set_target_properties(GLM::glm PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${GLM_INCLUDE_DIR}
)

add_executable(gles
  main.cpp
)
target_link_libraries(gles wlpp PkgConfig::egl PkgConfig::glesv2 GLM::glm)
