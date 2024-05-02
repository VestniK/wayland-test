find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

cpp_unit(
  NAME corort
  STD cxx_std_23
  LIBS
    asio::asio
    fmt::fmt
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
