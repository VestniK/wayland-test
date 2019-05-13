#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <wayland/gles2/triangular_net.hpp>

using Catch::Generators::random;
using namespace fmt::literals;

namespace Catch {

template <>
struct StringMaker<triangular::point> {
  static std::string convert(triangular::point pt) {
    return "{{tx: {}, ty: {}}}"_format(pt.tx, pt.ty);
  }
};

} // namespace Catch

TEST_CASE("triangular zero point is cartesian zero point", "[triangular]") {
  CHECK(triangular::to_cartesian({0, 0}) == glm::vec2{0.});
}

TEST_CASE("triangular to cartesian", "[triangular]") {
  const int tx = GENERATE(take(15, random(-50, 50)));
  const int ty = GENERATE(take(15, random(-50, 50)));
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

TEST_CASE("hexagon_net::cell_center returns zero point for 0,0 cell",
    "[hexagon_net]") {
  CHECK(hexagon_net::cell_center(0, 0) == triangular::point{0, 0});
}

TEST_CASE("hexagon_net::cell_center", "[hexagon_net]") {
  const int m = GENERATE(range(-3, 3));
  const int n = GENERATE(range(-3, 3));
  CAPTURE(m, n);

  SECTION("adjacent cells from the same column have common upper edge") {
    CHECK(hexagon_net::cell_center(m, n) + triangular::point{0, 1} ==
          hexagon_net::cell_center(m, n + 1) + triangular::point{1, -1});
    CHECK(hexagon_net::cell_center(m, n) + triangular::point{-1, 1} ==
          hexagon_net::cell_center(m, n + 1) + triangular::point{0, -1});
  }
}
