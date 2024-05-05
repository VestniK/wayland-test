find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)
find_package(mp-units REQUIRED)

cpp_unit(
  NAME scene
  STD cxx_std_23
  LIBS
    util
    geom
    memtricks
    gamepad-types
    img
    asio::asio
    glm::glm
    mp-units::si
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
    fmt::fmt
    mp-units::core-fmt
  TEST_ARGS --order rand --rng-seed time
)
