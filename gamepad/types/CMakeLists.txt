cpp_unit(
  NAME gamepad-types
  STD cxx_std_23
  LIBS
    util
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --rng-seed time
)
