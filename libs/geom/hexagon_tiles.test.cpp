#include <algorithm>
#include <array>
#include <numeric>

#include <mp-units/format.h>
#include <mp-units/systems/si.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <fmt/format.h>

#include <libs/geom/hexagon_tiles.hpp>

using namespace fmt::literals;
using namespace mp_units::si::unit_symbols;

namespace test {

namespace {

constexpr unsigned vertixies_in_triangle = 3;
constexpr unsigned triangles_in_hexagon = 6;
constexpr unsigned vertixies_in_hexagon =
    vertixies_in_triangle * triangles_in_hexagon;

template <size_t N>
std::array<glm::vec2, vertixies_in_triangle * N> get_triangles(
    const hexagon_tiles<glm::vec2>& land, size_t start_offset) noexcept {
  std::array<glm::vec2, vertixies_in_triangle * N> triangles_set;
  const auto indexes = std::span<const unsigned>{land.indexes()}.subspan(
      start_offset, vertixies_in_triangle * N);
  std::transform(indexes.begin(), indexes.end(), triangles_set.begin(),
      [vert = land.verticies()](unsigned idx) { return vert[idx]; });
  return triangles_set;
}

struct box {
  glm::vec2 min{std::numeric_limits<float>::max()};
  glm::vec2 max{std::numeric_limits<float>::min()};

  [[nodiscard]] float length() const noexcept {
    return max.x > min.x ? max.x - min.x : 0.f;
  }
  [[nodiscard]] float width() const noexcept {
    return max.y > min.y ? max.y - min.y : 0.f;
  }
};

box expand(box b, glm::vec2 pt) noexcept {
  b.min.x = std::min(b.min.x, pt.x);
  b.min.y = std::min(b.min.y, pt.y);

  b.max.x = std::max(b.max.x, pt.x);
  b.max.y = std::max(b.max.y, pt.y);
  return b;
}

template <typename InputIt>
box bounding_box(InputIt first, InputIt last) noexcept {
  return std::accumulate(first, last, box{}, expand);
}

} // namespace

TEST_CASE("generate_flat_landscape", "[landscape]") {
  const auto columns = GENERATE(range(1, 4));
  const auto rows = GENERATE(range(2, 4));
  const auto radius = (10.f * cm) * GENERATE(range(9, 11));
  INFO(fmt::format("{}x{} hexagons of size {}", columns, rows, radius));
  const auto land = hexagon_tiles<glm::vec2>::generate(
      radius, columns, rows, std::identity{});
  auto tiles_unit = hexagon_tiles<glm::vec2>::radius_t::unit;

  SECTION("covers flat rectangular area") {
    auto vert = land.verticies();
    const box landscape_bounds = bounding_box(vert.begin(), vert.end());
    SECTION("mesh has proper x dimensions") {
      CHECK(landscape_bounds.length() ==
            Catch::Approx((2 + (1 + std::cos(M_PI / 3)) * (columns - 1)) *
                          radius.numerical_value_in(tiles_unit)));
    }
    SECTION("mesh has proper y dimensions") {
      CHECK(landscape_bounds.width() ==
            Catch::Approx(rows * 2 * radius.numerical_value_in(tiles_unit) *
                          std::sin(M_PI / 3)));
    }
  }

  SECTION("generates set of hexagons") {

    REQUIRE(land.indexes().size() % vertixies_in_hexagon == 0);

    for (size_t index_set_start = 0; index_set_start < land.indexes().size();
         index_set_start += vertixies_in_hexagon) {
      std::array<glm::vec2, vertixies_in_hexagon> triangles_set =
          get_triangles<triangles_in_hexagon>(land, index_set_start);

      SECTION(
          fmt::format("triangles {} - {} of {} forms hexagon", index_set_start,
              index_set_start + vertixies_in_hexagon, land.indexes().size())) {
        for (size_t triangle_offset = 0; triangle_offset < triangles_set.size();
             triangle_offset += vertixies_in_triangle) {
          std::span<const glm::vec2, vertixies_in_triangle> triangle{
              triangles_set.data() + triangle_offset, vertixies_in_triangle};

          SECTION(fmt::format("triangle {} is equilateral",
              triangle_offset / triangles_in_hexagon)) {
            const auto equilateral_triangle_sides_dot_prod =
                (0.5 * radius * radius)
                    .numerical_value_in(tiles_unit * tiles_unit);
            CHECK(glm::dot(
                      triangle[1] - triangle[0], triangle[2] - triangle[0]) ==
                  Catch::Approx(equilateral_triangle_sides_dot_prod));
            CHECK(glm::dot(
                      triangle[0] - triangle[1], triangle[2] - triangle[1]) ==
                  Catch::Approx(equilateral_triangle_sides_dot_prod));
            CHECK(glm::dot(
                      triangle[0] - triangle[2], triangle[1] - triangle[2]) ==
                  Catch::Approx(equilateral_triangle_sides_dot_prod));
          }

          SECTION("there are 7 unique points (center and hexagon perimeter)") {
            std::sort(triangles_set.begin(), triangles_set.end(),
                [](glm::vec2 l, glm::vec2 r) {
                  return std::tie(l.x, l.y) < std::tie(r.x, r.y);
                });
            auto it = std::unique(triangles_set.begin(), triangles_set.end());
            CHECK(std::distance(triangles_set.begin(), it) == 7);
          }
        }
      }
    }
  }
}

} // namespace test
