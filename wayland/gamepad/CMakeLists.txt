add_library(gamepad OBJECT)
target_sources(gamepad PRIVATE
  udev.cpp
)
target_compile_features(gamepad PUBLIC cxx_std_20)
target_include_directories(gamepad PUBLIC ${PROJECT_SOURCE_DIR})
if (WAYLAND_TEST_WERROR)
  target_compile_options(gamepad PUBLIC -Werror=all)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
target_link_libraries(gamepad PUBLIC
  PkgConfig::UDEV
  fmt::fmt
  spdlog::spdlog
)
