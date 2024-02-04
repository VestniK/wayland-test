find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

find_package(PkgConfig)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

cpp_unit(
  NAME wayland-test
  STD cxx_std_23
  LIBS
    spdlog::spdlog
    PkgConfig::SYSTEMD
    wlwnd
    util
    gles2
    scene
    corort
    gamepad
    fmt::fmt
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)

target_sources(wayland-test PRIVATE
  ${CMAKE_SOURCE_DIR}/conanfile.txt
  ${CMAKE_SOURCE_DIR}/.clang-format
  ${CMAKE_SOURCE_DIR}/.clangd
)
add_custom_command(TARGET wayland-test POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:wayland-test> $<TARGET_FILE:wayland-test>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:wayland-test>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:wayland-test>.dbg $<TARGET_FILE:wayland-test>
)
