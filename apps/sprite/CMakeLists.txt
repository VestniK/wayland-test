find_package(asio REQUIRED)
find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(thinsys REQUIRED)

cpp_unit(
  NAME sprite
  STD cxx_std_23
  LIBS
    asio::asio
    cli
    corort
    fmt::fmt
    img
    sfx
    spdlog::spdlog
    thinsys-io
    wlwnd
    xdg
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)

find_program(OBJCOPY NAMES objcopy llvm-objcopy REQUIRED)

target_sources(sprite PRIVATE
  ${CMAKE_SOURCE_DIR}/conanfile.py
  ${CMAKE_SOURCE_DIR}/.clang-format
)
add_custom_command(TARGET sprite POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:sprite> $<TARGET_FILE:sprite>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:sprite>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:sprite>.dbg $<TARGET_FILE:sprite>
)
zip_sfx(sprite
  images/head.png
)
