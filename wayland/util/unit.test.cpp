#include <catch2/catch.hpp>

#include <wayland/util/unit.hpp>

TEST_CASE("unit conversions", "[unit]") {
  SECTION("meters are converted to centimeters") {
    centimeters len{meters{5}};
    REQUIRE(len.count() == 500);
  }
  SECTION("centimeters are converted to millimeters") {
    millimeters len{centimeters{7}};
    REQUIRE(len.count() == 70);
  }
}
