find_program(ZIP NAMES zip REQUIRED)

function(zip_sfx Tgt)
    add_custom_command(TARGET ${Tgt} POST_BUILD
      COMMAND ${ZIP} -0 "${CMAKE_CURRENT_BINARY_DIR}/tmp.zip" ${ARGN}
      COMMAND ${CMAKE_COMMAND} -E cat "${CMAKE_CURRENT_BINARY_DIR}/tmp.zip" >> "$<TARGET_FILE:${Tgt}>"
      COMMAND ${ZIP} -A "$<TARGET_FILE:${Tgt}>"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
target_sources(${Tgt} PRIVATE ${ARGN})
set_property(SOURCE ${ARGN} PROPERTY HEADER_FILE_ONLY On)
endfunction()
