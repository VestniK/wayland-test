add_library(gles2 OBJECT)
target_sources(gles2 PRIVATE
  gl_resource.cpp
  gl_resource.hpp
  renderer.cpp
  renderer.hpp
)
target_include_directories(gles2 PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_features(gles2 PUBLIC cxx_std_17)
target_link_libraries(gles2 PUBLIC
  util
  OpenGL::OpenGL
  CONAN_PKG::glm
  CONAN_PKG::gsl_microsoft
)

add_executable(gl_resource.test gl_resource.test.cpp)
target_link_libraries(gl_resource.test
PRIVATE
  gles2
  CONAN_PKG::catch2
)
add_test(NAME gl_resource COMMAND gl_resource.test --order rand)

