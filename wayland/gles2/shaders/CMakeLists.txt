find_program(GPP_PATH gpp)
add_executable(GPP::gpp IMPORTED)
set_target_properties(GPP::gpp PROPERTIES IMPORTED_LOCATION ${GPP_PATH})

# write header
file(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp
  "namespace shaders {\n"
)
file(
  APPEND ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp
  "extern const char* const main_fsl;\n"
)
file(
APPEND ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp
"extern const char* const main_vsl;\n"
)
file(
  APPEND ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp
  "}"
)

# write cpp template
file(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/main.fsl.gpp
  "#include \"shaders.hpp\"\n"
  "namespace shaders {\n"
)
file(
  APPEND ${CMAKE_CURRENT_BINARY_DIR}/main.fsl.gpp
  "const char* const main_fsl = R\"(\n@include(${CMAKE_CURRENT_SOURCE_DIR}/main.fsl)\n)\";\n"
)
file(
  APPEND ${CMAKE_CURRENT_BINARY_DIR}/main.fsl.gpp
  "const char* const main_vsl = R\"(\n@include(${CMAKE_CURRENT_SOURCE_DIR}/main.vsl)\n)\";\n"
)
file(
  APPEND ${CMAKE_CURRENT_BINARY_DIR}/main.fsl.gpp
  "}"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/main.fsl.cpp"
  COMMAND GPP::gpp -U '@' '' '\(' ',' '\)' '\(' '\)' '\#' '\\\\' main.fsl.gpp -o main.fsl.cpp
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/main.fsl" "${CMAKE_CURRENT_BINARY_DIR}/main.fsl.gpp"
)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/main.fsl.cpp" PROPERTIES GENERATED On)
add_library(shaders STATIC
  "${CMAKE_CURRENT_BINARY_DIR}/main.fsl.cpp"

  main.vsl
  phong.fsl
  main.fsl
)
target_compile_features(shaders PRIVATE cxx_std_17)
target_include_directories(shaders PUBLIC ${PROJECT_BINARY_DIR})
