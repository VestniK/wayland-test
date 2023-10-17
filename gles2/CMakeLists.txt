add_subdirectory(shaders)

find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)

cpp_unit(
  NAME gles2
  STD cxx_std_23
  LIBS
    util
    scene
    shaders
    OpenGL::OpenGL
    glm::glm
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
    fmt::fmt
  TEST_ARGS --order rand --rng-seed time
)
