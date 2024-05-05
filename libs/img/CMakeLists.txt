find_package(Catch2 REQUIRED)
find_package(PNG NO_MODULE REQUIRED)
find_package(spdlog REQUIRED)
find_package(thinsys REQUIRED)

cpp_unit(
  NAME img
  STD cxx_std_23
  LIBS
    geom
    thinsys-io
    PNG::PNG
    spdlog::spdlog
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
