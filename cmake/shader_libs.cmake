find_program(GPP_PATH gpp)
if (NOT GPP_PATH)
  message(FATAL_ERROR "gpp executable not found")
endif()
add_executable(GPP::gpp IMPORTED)
set_target_properties(GPP::gpp PROPERTIES IMPORTED_LOCATION ${GPP_PATH})

function(add_shaders_lib)
  set(options)
  set(one_value_args NAME)
  set(multi_value_args PROGRAMS SOURCES)
  cmake_parse_arguments(SHADER_LIB
    "${options}" "${one_value_args}" "${multi_value_args}"
    ${ARGN}
  )
  set(_lib_hdr "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_LIB_NAME}.hpp")
  set(_lib_gpp "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_LIB_NAME}.gpp")
  set(_lib_src "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_LIB_NAME}.cpp")

  foreach(_src ${SHADER_LIB_SOURCES})
    get_filename_component(_src_dep "${_src}" ABSOLUTE)
    list(APPEND _dependencies ${_src_dep})
  endforeach()

  # write header
  file(WRITE ${_lib_hdr} "#pragma once\n")
  file(APPEND ${_lib_hdr} "namespace ${SHADER_LIB_NAME} {\n")
  foreach(_prog ${SHADER_LIB_PROGRAMS})
    get_filename_component(_prog_name "${_prog}" NAME_WE)

    file(APPEND ${_lib_hdr} "extern const char* const ${_prog_name}_vert;\n")
    file(APPEND ${_lib_hdr} "extern const char* const ${_prog_name}_frag;\n\n")
  endforeach()
  file(APPEND ${_lib_hdr} "}")

  # write source gpp
  file(WRITE ${_lib_gpp} "#include \"${SHADER_LIB_NAME}.hpp\"\n")
  file(APPEND ${_lib_gpp} "namespace ${SHADER_LIB_NAME} {\n")
  foreach(_prog ${SHADER_LIB_PROGRAMS})
    get_filename_component(_prog_vert "${_prog}.vert" ABSOLUTE)
    get_filename_component(_prog_frag "${_prog}.frag" ABSOLUTE)
    get_filename_component(_prog_name "${_prog}" NAME_WE)

    list(APPEND _dependencies ${_prog_vert})
    list(APPEND _dependencies ${_prog_frag})

    file(APPEND ${_lib_gpp} "extern const char* const ${_prog_name}_vert = R\"(\n")
    file(APPEND ${_lib_gpp} "@include(${_prog_vert})\n")
    file(APPEND ${_lib_gpp} ")\";\n")
    file(APPEND ${_lib_gpp} "extern const char* const ${_prog_name}_frag = R\"(\n")
    file(APPEND ${_lib_gpp} "@include(${_prog_frag})\n")
    file(APPEND ${_lib_gpp} ")\";\n")
  endforeach()
  file(APPEND ${_lib_gpp} "}")

  add_custom_command(
    OUTPUT "${_lib_src}"
    COMMAND GPP::gpp -U '@' '' '\(' ',' '\)' '\(' '\)' '\#' '\\\\' ${_lib_gpp} -o ${_lib_src}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS
      ${_dependencies}
      ${_lib_gpp}
  )
  set_source_files_properties("${_lib_src}" PROPERTIES GENERATED On)
  set_source_files_properties("${_lib_hdr}" PROPERTIES GENERATED On)

  add_library(${SHADER_LIB_NAME} STATIC
    "${_lib_src}"

    # Shader sources
    ${_dependencies}
  )
  target_compile_features(${SHADER_LIB_NAME} PRIVATE cxx_std_20)
  target_include_directories(${SHADER_LIB_NAME} PUBLIC ${PROJECT_BINARY_DIR})
endfunction()
