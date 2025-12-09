find_package(Catch2 REQUIRED)

cpp_unit(
  NAME testprinters
  STD cxx_std_23
  LIBS
    Catch2::Catch2
)
