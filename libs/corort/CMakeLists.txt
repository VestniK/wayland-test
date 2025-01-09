find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

find_package(PkgConfig)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

cpp_unit(
  NAME corort
  STD cxx_std_23
  LIBS
    asio::asio
    fmt::fmt
    spdlog::spdlog
    PkgConfig::SYSTEMD
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
