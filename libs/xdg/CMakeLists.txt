find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

cpp_unit(
  NAME xdg
  STD cxx_std_23
  LIBS
  TEST_LIBS
    fmt::fmt
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
