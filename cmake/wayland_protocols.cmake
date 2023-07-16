find_package(Wayland REQUIRED)

function(target_wl_protocol Tgt)
  set(opts "")
  set(oneval_args NAME URL SHA256)
  set(multyval_args "")
  cmake_parse_arguments(WL_PROTOCOL
    "${opts}"
    "${oneval_args}"
    "${multyval_args}"
    ${ARGN}
  )

  set(_OUT_XML ${CMAKE_CURRENT_BINARY_DIR}/${WL_PROTOCOL_NAME}.xml)
  set(_OUT_HDR ${CMAKE_CURRENT_BINARY_DIR}/${WL_PROTOCOL_NAME}.h)
  set(_OUT_SRC ${CMAKE_CURRENT_BINARY_DIR}/${WL_PROTOCOL_NAME}.c)
  file(DOWNLOAD
    ${WL_PROTOCOL_URL} ${_OUT_XML}
    EXPECTED_HASH SHA256=${WL_PROTOCOL_SHA256}
  )
  add_custom_command(OUTPUT ${_OUT_HDR}
    COMMAND Wayland::scanner client-header ${_OUT_XML} ${_OUT_HDR}
    COMMAND Wayland::scanner private-code ${_OUT_XML} ${_OUT_SRC}
    DEPENDS ${_OUT_XML}
  )
  set_source_files_properties(
    ${_OUT_SRC}
    ${_OUT_HDR}
    PROPERTIES
      GENERATED ON
  )
  target_sources(${Tgt} PRIVATE
    ${_OUT_SRC}
    ${_OUT_HDR}
  )
  target_include_directories(${Tgt} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endfunction()
