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
  TEST_ARGS --rng-seed time
)

target_compile_definitions(corort PUBLIC
  # FIXME: For QtCreator clang based code completion only
  ASIO_HAS_CO_AWAIT
  ASIO_HAS_STD_COROUTINE
)
