function(add_catch_test)
  set(options)
  set(one_value_args NAME)
  set(multi_value_args SOURCES)
  cmake_parse_arguments(CATCH_TEST
    "${options}" "${one_value_args}" "${multi_value_args}"
    ${ARGN}
  )
  add_executable(${CATCH_TEST_NAME} ${CATCH_TEST_SOURCES})
  target_link_libraries(${CATCH_TEST_NAME}
  PRIVATE
    CONAN_PKG::catch2
  )
  add_test(NAME ${CATCH_TEST_NAME} COMMAND ${CATCH_TEST_NAME} --order rand)
endfunction()
