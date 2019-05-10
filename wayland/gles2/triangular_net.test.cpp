#include <catch2/catch.hpp>

#include <wayland/gles2/triangular_net.hpp>

using Catch::Generators::random;

TEST_CASE("triangular zero point is cartesian zero point", "[triangular_net]") {
  CHECK(triangular::to_cartesian({0, 0}) == glm::vec2{0.});
}

TEST_CASE("triangular to cartesian", "[triangular_net]") {
  auto tx = GENERATE(take(15, random(-50, 50)));
  auto ty = GENERATE(take(15, random(-50, 50)));
  INFO("tx: " << tx << "; ty: " << ty);

  SECTION("x coordinate calculated properly") {
    CHECK_THAT(triangular::to_cartesian({tx, ty}).x,
        Catch::WithinAbs(tx + ty * std::cos(float(M_PI) / 3), 0.0001));
  }

  SECTION("y coordinate calculated properly") {
    CHECK_THAT(triangular::to_cartesian({tx, ty}).y,
        Catch::WithinAbs(ty * std::sin(float(M_PI) / 3), 0.0001));
  }
}
