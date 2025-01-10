#include "region.hpp"

#include <catch2/catch_test_macros.hpp>

SCENARIO("Calculation of reguin for subspan of given span") {
  GIVEN("span and it's subspan") {
    int data[] = {1, 2, 3, 4, 5, 7, 8, 9, 10};
    auto span = as_bytes(std::span{data});
    std::span subspan = span.subspan(2, 4);

    WHEN("subspan region is calculated") {
      const auto region = subspan_region(span, subspan);

      THEN("region start points to subspan first element") {
        REQUIRE(span.data() + region.offset == subspan.data());
      }

      THEN("region length is same as subspan size") { REQUIRE(region.len == subspan.size()); }
    }
  }
}
