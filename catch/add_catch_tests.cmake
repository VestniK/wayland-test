function(add_catch_tests)
  set(options)
  set(one_value_args)
  set(multi_value_args SOURCES LIBS)
  cmake_parse_arguments(CATCH_TESTS
    "${options}" "${one_value_args}" "${multi_value_args}"
    ${ARGN}
  )
  foreach(src ${CATCH_TESTS_SOURCES})
    get_filename_component(target_name ${src} NAME_WLE)
    get_filename_component(test_name ${src} NAME_WE)
    add_executable(${target_name} ${src})
    target_link_libraries(${target_name}
    PRIVATE
      ${CATCH_TESTS_LIBS}
      catch_main
    )
    add_test(NAME ${test_name} COMMAND ${target_name} --order rand)
  endforeach()
endfunction()
