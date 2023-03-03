find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

cpp_unit(
  NAME util
  STD cxx_std_20
  LIBS
    asio::asio
  TEST_LIBS
    fmt::fmt
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --rng-seed time
)

target_compile_definitions(util PUBLIC
  # FIXME: For QtCreator clang based code completion only
  ASIO_HAS_CO_AWAIT
  ASIO_HAS_STD_COROUTINE
)
target_compile_options(util PUBLIC -Wno-error=interference-size)

