add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp
  COMMAND tools::shaders2consts -o ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp --header ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp -d ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp.d -i basic_frag.glsl -i basic_vert.glsl
  DEPENDS basic_frag.glsl basic_vert.glsl tools::shaders2consts
  DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp.d
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/shaders.cpp PROPERTY GENERATED On)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp PROPERTY GENERATED On)

add_library(shaders STATIC)
target_sources(shaders PRIVATE
  shaders.cpp
  shaders.hpp
)
target_compile_features(shaders PRIVATE cxx_std_20)
