find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(thinsys REQUIRED)

cpp_unit(
  NAME shaders2consts
  STD cxx_std_23
  LIBS
    util
    thinsys-io
    fmt::fmt
  TEST_LIBS
    testmatchers
    testprinters
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)

set_property(TARGET shaders2consts PROPERTY EXCLUDE_FROM_ALL On)
include(ExternalProject)
ExternalProject_Add(
  project_tools
  SOURCE_DIR ${PROJECT_SOURCE_DIR}
  CMAKE_ARGS
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=${PROJECT_SOURCE_DIR}/cmake/conan_provider.cmake
    -DCMAKE_BUILD_TYPE=Release
    -DDCMAKE_CONFIGURATION_TYPES=Release
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release -t $<TARGET_PROPERTY:project_tools,TOOL_TARGETS>
  BUILD_ALWAYS On
  BUILD_BYPRODUCTS <BINARY_DIR>/bin/shaders2consts
  EXCLUDE_FROM_ALL On
  USES_TERMINAL_BUILD On
  STEP_TARGETS build
)
ExternalProject_Get_Property(project_tools BINARY_DIR)
set_property(TARGET project_tools APPEND PROPERTY TOOL_TARGETS shaders2consts)
add_executable(tools::shaders2consts IMPORTED GLOBAL)
set_property(TARGET tools::shaders2consts PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/bin/shaders2consts)
add_dependencies(tools::shaders2consts project_tools-build)
