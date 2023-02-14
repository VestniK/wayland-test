add_library(gamepad OBJECT)
target_sources(gamepad PRIVATE
  udev_gamepads.cpp
  udev_gamepads.hpp
)
target_compile_features(gamepad PUBLIC cxx_std_20)
target_include_directories(gamepad PUBLIC ${PROJECT_SOURCE_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
target_link_libraries(gamepad PUBLIC
  PkgConfig::UDEV
  fmt::fmt
  spdlog::spdlog
  asio::asio
)
target_compile_definitions(gamepad PRIVATE
  # FIXME: For QtCreator clang based code completion only
  ASIO_HAS_CO_AWAIT
  ASIO_HAS_STD_COROUTINE
)
