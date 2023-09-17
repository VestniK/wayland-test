find_package(Catch2 REQUIRED)
find_package(PNG NO_MODULE REQUIRED)
find_package(spdlog REQUIRED)
find_package(Microsoft.GSL REQUIRED)

cpp_unit(
  NAME img
  STD cxx_std_23
  LIBS
    PNG::PNG
    spdlog::spdlog
    Microsoft.GSL::GSL
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --rng-seed time
)
