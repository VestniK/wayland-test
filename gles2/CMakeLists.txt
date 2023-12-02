add_subdirectory(shaders)

find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(Tracy)

cpp_unit(
  NAME gles2
  STD cxx_std_23
  LIBS
    util
    scene
    shaders
    OpenGL::OpenGL
    glm::glm
    Tracy::TracyClient
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
    fmt::fmt
  TEST_ARGS --order rand --rng-seed time
)

option(TRACY_ENABLE "Enable tracy telemetry" Off)
if (TRACY_ENABLE)
  target_compile_definitions(gles2 PRIVATE TRACY_ENABLE)
endif()
