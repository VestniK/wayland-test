find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(asio REQUIRED)

find_package(PkgConfig)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

cpp_unit(
  NAME castle
  STD cxx_std_23
  LIBS
    asio::asio
    spdlog::spdlog
    PkgConfig::SYSTEMD
    wlwnd
    cli
    xdg
    vk
    util
    corort
    fmt::fmt
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)

target_sources(castle PRIVATE
  ${CMAKE_SOURCE_DIR}/conanfile.txt
  ${CMAKE_SOURCE_DIR}/.clang-format
  ${CMAKE_SOURCE_DIR}/.clangd
)
add_custom_command(TARGET castle POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:castle> $<TARGET_FILE:castle>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:castle>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:castle>.dbg $<TARGET_FILE:castle>
)
