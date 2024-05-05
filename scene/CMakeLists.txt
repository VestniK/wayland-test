find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)

cpp_unit(
  NAME scene
  STD cxx_std_23
  LIBS
    anime
    geom
    memtricks
    gamepad-types
    img
    asio::asio
    glm::glm
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
    fmt::fmt
    mp-units::core-fmt
  TEST_ARGS --order rand --rng-seed time
)
