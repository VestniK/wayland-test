add_library(local_catch_main STATIC)
target_sources(local_catch_main PRIVATE
  catch_main.cpp
)
target_link_libraries(local_catch_main PUBLIC
  CONAN_PKG::catch2
)
