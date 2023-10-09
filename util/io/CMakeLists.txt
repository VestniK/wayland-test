find_package(Catch2 REQUIRED)

cpp_unit(
  NAME uio
  STD cxx_std_23
  LIBS
    util
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --rng-seed time
)
