include(tool_targets)

find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(thinsys REQUIRED)

cpp_unit(
  NAME shaders2consts
  STD cxx_std_23
  LIBS
    util
    thinsys-io
    fmt::fmt
  TEST_LIBS
    testmatchers
    testprinters
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
tool_target(shaders2consts)
