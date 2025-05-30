include(target_shaders)

find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(asio REQUIRED)
find_package(Tracy REQUIRED)

cpp_unit(
  NAME colorcube
  STD cxx_std_23
  LIBS
    anime
    asio::asio
    spdlog::spdlog
    wlwnd
    xdg
    geom
    eglctx
    cli
    gles2
    corort
    memtricks
    gamepad
    gamepad-types
    fmt::fmt
    Tracy::TracyClient
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
target_shaders(colorcube.lib
  INCLUDE_DIR shaders
  SOURCES
    shaders/basic_vert.glsl
    shaders/basic_frag.glsl
)

target_sources(colorcube PRIVATE
  ${CMAKE_SOURCE_DIR}/conanfile.py
  ${CMAKE_SOURCE_DIR}/.clang-format
)
add_custom_command(TARGET colorcube POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:colorcube> $<TARGET_FILE:colorcube>.dbg
  COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:colorcube>
  COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:colorcube>.dbg $<TARGET_FILE:colorcube>
)
option(TRACY_ENABLE "Enable tracy telemetry" Off)
if (TRACY_ENABLE)
  target_compile_definitions(colorcube PRIVATE TRACY_ENABLE)
endif()
