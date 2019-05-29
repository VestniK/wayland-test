add_library(util OBJECT)
target_sources(util PRIVATE
  concat.hpp
  geom.hpp
  get_option.cpp
  get_option.hpp
  member.hpp
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

add_catch_test(
NAME
  util.test
SOURCES
  get_option.test.cpp
)
target_link_libraries(util.test PRIVATE util)
