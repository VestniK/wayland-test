add_headeronly_lib(io
  file.hpp
  shmem.hpp
  xdg.hpp
)
target_include_directories(io INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(io INTERFACE
  util
  $<$<CXX_COMPILER_ID:Clang>:c++experimental>
  $<$<CXX_COMPILER_ID:GNU>:stdc++fs>
)
