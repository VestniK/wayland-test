add_library(catch_main OBJECT)

target_sources(catch_main
PRIVATE
  main.cpp

  add_catch_tests.cmake
)
target_compile_definitions(catch_main PRIVATE CATCH_CONFIG_MAIN)
target_link_libraries(catch_main PUBLIC CONAN_PKG::Catch2)
target_compile_features(catch_main PUBLIC cxx_std_17)
