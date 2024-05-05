find_package(glm REQUIRED)

cpp_unit(
  NAME gamepad-types
  STD cxx_std_23
  LIBS
    glm::glm
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
