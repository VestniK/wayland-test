include(ExternalProject)

function(tool_target Tgt)
  if (NOT TARGET project_tools)
    ExternalProject_Add(
      project_tools
      SOURCE_DIR ${PROJECT_SOURCE_DIR}
      CMAKE_ARGS
        -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=${PROJECT_SOURCE_DIR}/cmake/conan_provider.cmake
        -DCMAKE_BUILD_TYPE=Release
        -DDCMAKE_CONFIGURATION_TYPES=Release
      BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release -t $<TARGET_PROPERTY:project_tools,TOOL_TARGETS>
      BUILD_ALWAYS On
      BUILD_BYPRODUCTS <BINARY_DIR>/build_tools/${Tgt} # TODO: how to append byproducts here for multiple invocaions?
      EXCLUDE_FROM_ALL On
      USES_TERMINAL_BUILD On
      STEP_TARGETS build
    )
  endif()

  set_property(TARGET ${Tgt} PROPERTY EXCLUDE_FROM_ALL On)
  set_property(TARGET ${Tgt} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build_tools)
  set_property(TARGET project_tools APPEND PROPERTY TOOL_TARGETS ${Tgt})

  set(ToolTgt tools::${Tgt})
  add_executable(${ToolTgt} IMPORTED GLOBAL)
  ExternalProject_Get_Property(project_tools BINARY_DIR)
  set_property(TARGET ${ToolTgt} PROPERTY IMPORTED_LOCATION ${BINARY_DIR}/build_tools/${Tgt})
  add_dependencies(${ToolTgt} project_tools-build)
endfunction()
