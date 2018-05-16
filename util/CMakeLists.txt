add_headeronly_lib(util
  bitmask.hpp
  util.hpp
)
target_include_directories(util INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
