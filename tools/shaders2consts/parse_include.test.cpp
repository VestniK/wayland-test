#include <concepts>
#include <expected>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <testing/matchers/expected.hpp>
#include <testing/printers/expected.hpp>

#include "parse_include.hpp"

using namespace std::literals;
namespace fs = std::filesystem;

CATCH_REGISTER_ENUM(include_parse_error, include_parse_error::non_include_line,
    include_parse_error::missing_open_quot,
    include_parse_error::missing_close_quot)

SCENARIO("successfull parsing") {
  auto ln = GENERATE("@include \"some/file.txt\""s,
      " @include \"some/file.txt\""s, "\t@include \"some/file.txt\""s,
      "@include \"some/file.txt\"  \t"s, "    @include \"some/file.txt\"  \t"s,
      "    @include\t\"some/file.txt\"  \t"s,
      "    @include\t  \"some/file.txt\"  \t"s);
  GIVEN("valid @include line: '" + ln + '\'') {
    THEN("it parses to include path") {
      REQUIRE_THAT(parse_include(ln), is_expected("some/file.txt"));
    }
  }
}
