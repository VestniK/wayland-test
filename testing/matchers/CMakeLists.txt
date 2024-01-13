find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

cpp_unit(
  NAME testmatchers
  STD cxx_std_23
  LIBS
    fmt::fmt
    Catch2::Catch2
)
