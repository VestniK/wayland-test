find_package(Catch2 REQUIRED)
find_package(glm REQUIRED)
find_package(mp-units REQUIRED)

cpp_unit(
  NAME geom
  STD cxx_std_23
  LIBS
    glm::glm
    mp-units::systems
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
    mp-units::core
  TEST_ARGS --order rand --rng-seed time
)
