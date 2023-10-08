find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)

cpp_unit(
  NAME scene
  STD cxx_std_23
  LIBS
    util
    gamepad-types
    glm::glm
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
    fmt::fmt
  TEST_ARGS --rng-seed time
)
