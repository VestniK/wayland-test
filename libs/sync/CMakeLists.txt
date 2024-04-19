find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

cpp_unit(
  NAME sync
  STD cxx_std_23
  LIBS
    asio::asio
  TEST_LIBS
    fmt::fmt
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)

target_compile_options(sync INTERFACE -Wno-error=interference-size -Wno-interference-size)
