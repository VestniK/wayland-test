add_library(coro-rt OBJECT)
target_sources(coro-rt PRIVATE
  runtime.cpp
)
target_compile_features(coro-rt PUBLIC cxx_std_20)
target_include_directories(coro-rt PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(coro-rt
PUBLIC
  stdc++fs
  CONAN_PKG::asio
  CONAN_PKG::fmt
)
target_compile_definitions(coro-rt PUBLIC
  # FIXME: For QtCreator clang based code completion only
  ASIO_HAS_CO_AWAIT
  ASIO_HAS_STD_COROUTINE
)
if (WAYLAND_TEST_WERROR)
  target_compile_options(coro-rt PUBLIC -Werror=all)
endif()
