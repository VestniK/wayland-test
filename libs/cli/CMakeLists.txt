find_package(Catch2 REQUIRED)

add_library(cli)
target_sources(cli PUBLIC
  FILE_SET CXX_MODULES
  FILES
    cli.cppm
    get_option.cppm
    struct_args.cppm
)

add_executable(cli.test)
set_property(TARGET cli.test PROPERTY CXX_SCAN_FOR_MODULES On)
target_link_libraries(cli.test PRIVATE
  cli
  Catch2::Catch2
  Catch2::Catch2WithMain
)
target_sources(cli.test PRIVATE
  get_option.test.cpp
  struct_args.test.cpp
)
add_test(NAME cli.test-test COMMAND cli.test --order rand --rng-seed time)
