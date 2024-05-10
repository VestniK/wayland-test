find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS EGL)
find_package(spdlog REQUIRED)

cpp_unit(
  NAME eglctx
  STD cxx_std_23
  LIBS
    OpenGL::EGL
    spdlog::spdlog
    wlwnd
    Wayland::egl
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
    fmt::fmt
  TEST_ARGS --order rand --rng-seed time
)
