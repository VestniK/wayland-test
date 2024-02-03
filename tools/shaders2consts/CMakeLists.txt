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
  ${CMAKE_PROJECT_NAME}_tools
  SOURCE_DIR ${PROJECT_SOURCE_DIR}
  CMAKE_ARGS
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=${PROJECT_SOURCE_DIR}/cmake/conan_provider.cmake
    -DCMAKE_BUILD_TYPE=Release
    -DDCMAKE_CONFIGURATION_TYPES=Release
  CONFIGURE_HANDLED_BY_BUILD On
  EXCLUDE_FROM_ALL On
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  TEST_COMMAND ""
)

ExternalProject_Add_Step(${CMAKE_PROJECT_NAME}_tools shaders2consts_tool
  COMMAND ${CMAKE_COMMAND} --build . -t shaders2consts --config Release
  ALWAYS On
  USES_TERMINAL On
  BYPRODUCTS <BINARY_DIR>/bin/shaders2consts
  EXCLUDE_FROM_MAIN On
  WORKING_DIRECTORY <BINARY_DIR>
)
ExternalProject_Add_StepDependencies(${CMAKE_PROJECT_NAME}_tools shaders2consts_tool ${CMAKE_PROJECT_NAME}_tools)
ExternalProject_Add_StepTargets(${CMAKE_PROJECT_NAME}_tools shaders2consts_tool)
ExternalProject_Get_Property(${CMAKE_PROJECT_NAME}_tools BINARY_DIR)
add_executable(tools::shaders2consts IMPORTED GLOBAL)
set_property(TARGET tools::shaders2consts PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/bin/shaders2consts)
add_dependencies(tools::shaders2consts ${CMAKE_PROJECT_NAME}_tools-shaders2consts_tool)
