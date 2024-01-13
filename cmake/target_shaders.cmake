function(target_shaders Tgt Incdir)
  foreach(Arg ${ARGN})
    list(APPEND InputArgs "-i" ${Arg})
  endforeach()

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp
    COMMAND tools::shaders2consts -o ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp --header ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp -d ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp.d -I ${Incdir} ${InputArgs}
    DEPENDS ${ARGN} tools::shaders2consts
    DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp.d
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp PROPERTY GENERATED On)
  set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp PROPERTY GENERATED On)

  target_sources(${Tgt} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp
    ${ARGN}
  )
endfunction()
