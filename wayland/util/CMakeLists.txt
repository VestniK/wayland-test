add_library(util OBJECT)
target_sources(util PRIVATE
  concat.hpp
  geom.hpp
  get_option.cpp
  get_option.hpp
  member.hpp
  morton.hpp
  task_guard.hpp
  unit.hpp
  xdg.cpp
  xdg.hpp
)
target_compile_features(util PUBLIC cxx_std_20)
target_include_directories(util PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(util PUBLIC asio::asio)
target_compile_definitions(util PUBLIC
  # FIXME: For QtCreator clang based code completion only
  ASIO_HAS_CO_AWAIT
  ASIO_HAS_STD_COROUTINE
)

if (WAYLAND_TEST_WERROR)
  target_compile_options(util PUBLIC -Werror=all)
endif()

add_catch_test(
NAME
  util.test
SOURCES
  channel.test.cpp
  executors_environment.test.cpp
  executors_environment.test.hpp
  get_option.test.cpp
  morton.test.cpp
  task_guard.test.cpp
  unit.test.cpp
)
target_link_libraries(util.test PRIVATE util fmt::fmt)
