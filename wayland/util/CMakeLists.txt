add_library(util OBJECT)
target_sources(util PRIVATE
  geom.hpp
  get_option.cpp
  get_option.hpp
  xdg.cpp
  xdg.hpp
)
target_compile_features(util PUBLIC cxx_std_17)
target_include_directories(util PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(util
PUBLIC
  stdc++fs
  CONAN_PKG::gsl_microsoft
)
if (WAYLAND_TEST_WERROR)
  target_compile_options(util PUBLIC -Werror=all)
endif()

add_executable(get_option.test get_option.test.cpp)
target_link_libraries(get_option.test
PRIVATE
  util
  catch_main
)
add_test(NAME get_option COMMAND get_option.test --order rand)
