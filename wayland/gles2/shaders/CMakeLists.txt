find_program(GPP_PATH gpp)
add_executable(GPP::gpp IMPORTED)
set_target_properties(GPP::gpp PROPERTIES IMPORTED_LOCATION ${GPP_PATH})

file(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp
  "namespace shaders {\nextern const char* const main_fsl;\n}"
)
file(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/main.fsl.gpp
  "#include \"shaders.hpp\"\nnamespace shaders {\nconst char* const main_fsl=R\"(\n@include(${CMAKE_CURRENT_SOURCE_DIR}/main.fsl)\n)\";\n}"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/main.fsl.cpp"
  COMMAND GPP::gpp -U '@' '' '\(' ',' '\)' '\(' '\)' '\#' '\\\\' main.fsl.gpp -o main.fsl.cpp
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/main.fsl"
)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/main.fsl.cpp" PROPERTIES GENERATED On)
add_library(shaders STATIC "${CMAKE_CURRENT_BINARY_DIR}/main.fsl.cpp" main.fsl)
target_compile_features(shaders PRIVATE cxx_std_17)
target_include_directories(shaders PUBLIC ${PROJECT_BINARY_DIR})
