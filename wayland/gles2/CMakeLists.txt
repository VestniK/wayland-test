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
