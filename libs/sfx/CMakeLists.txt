find_package(Catch2 REQUIRED)
find_package(thinsys REQUIRED)

cpp_unit(
  NAME sfx
  STD cxx_std_23
  LIBS
    thinsys-io
    memtricks
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)

zip_sfx(sfx.test
  a.txt
  b.txt
)
