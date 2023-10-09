add_subdirectory(types)

find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)

cpp_unit(
  NAME gamepad
  STD cxx_std_23
  LIBS
    util
    uio
    PkgConfig::UDEV
    fmt::fmt
    spdlog::spdlog
    asio::asio
    gamepad-types
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --rng-seed time
)

target_compile_definitions(gamepad PUBLIC
  # FIXME: For QtCreator clang based code completion only
  ASIO_HAS_CO_AWAIT
  ASIO_HAS_STD_COROUTINE
)
option(TRACE_GAMEPAD_EVENTS Off "Enable all gamepad events tracing")
if (TRACE_GAMEPAD_EVENTS)
  target_compile_definitions(gamepad PRIVATE SPDLOG_ACTIVE_LEVEL=trace)
endif()
