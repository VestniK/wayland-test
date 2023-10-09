#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <fmt/format.h>

#include <scene/triangular_net.hpp>

using Catch::Generators::random;
using namespace fmt::literals;

namespace Catch {

template <>
struct StringMaker<triangular::point> {
  static std::string convert(triangular::point pt) {
    return fmt::format("{{x: {}, y: {}}}", pt.x, pt.y);
  }
};

} // namespace Catch

TEST_CASE("triangular zero point is cartesian zero point", "[triangular]") {
  CHECK(triangular::to_cartesian({0, 0}) == glm::vec2{0.});
}

TEST_CASE("triangular to cartesian", "[triangular]") {
  const int tx = GENERATE(take(15, random(-50, 50)));
  const int ty = GENERATE(take(15, random(-50, 50)));
  CAPTURE(tx, ty);

  SECTION("x coordinate calculated properly") {
    CHECK_THAT(triangular::to_cartesian({tx, ty}).x,
        Catch::Matchers::WithinAbs(tx + ty * std::cos(M_PI / 3), 0.0001));
  }

  SECTION("y coordinate calculated properly") {
    CHECK_THAT(triangular::to_cartesian({tx, ty}).y,
        Catch::Matchers::WithinAbs(ty * std::sin(M_PI / 3), 0.0001));
  }
}

TEST_CASE("hexagon_net::cell_center returns zero point for 0,0 cell",
    "[hexagon_net]") {
  CHECK(hexagon_net::cell_center(0, 0) == triangular::point{0, 0});
}

TEST_CASE("hexagon_net::cell_center", "[hexagon_net]") {
  using namespace hexagon_net;

  const int m = GENERATE(range(-3, 3));
  const int n = GENERATE(range(-3, 3));
  CAPTURE(
      m, n, cell_center(m, n), cell_center(m, n + 1), cell_center(m + 1, n));

  SECTION("adjacent cells from the same column have common upper border") {
    CHECK(cell_coord(cell_center(m, n), corner::top_left) ==
          cell_coord(cell_center(m, n + 1), corner::bottom_left));
    CHECK(cell_coord(cell_center(m, n), corner::top_right) ==
          cell_coord(cell_center(m, n + 1), corner::bottom_right));
  }

  /*
   * even odd even
   *       __
   *    __/  \__
   *   /  \__/  \
   *   \__/  \__/
   */
  SECTION("adjacent cells from the same row have proper common border") {
    CHECK(cell_coord(cell_center(m, n),
              m % 2 == 0 ? corner::top_right : corner::bottom_right) ==
          cell_coord(cell_center(m + 1, n), corner::midle_left));
    CHECK(cell_coord(cell_center(m, n), corner::midle_right) ==
          cell_coord(cell_center(m + 1, n),
              m % 2 == 0 ? corner::bottom_left : corner::top_left));
  }
}
