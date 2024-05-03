find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)

cpp_unit(
  NAME geom
  STD cxx_std_23
  LIBS
    glm::glm
  TEST_LIBS
    fmt::fmt
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
