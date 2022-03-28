#include <array>

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <wayland/gles2/landscape.hpp>

using namespace fmt::literals;

namespace test {

namespace {

constexpr unsigned vertixies_in_triangle = 3;
constexpr unsigned triangles_in_hexagon = 6;
constexpr unsigned vertixies_in_hexagon =
    vertixies_in_triangle * triangles_in_hexagon;

template <size_t N>
std::array<glm::vec3, vertixies_in_triangle * N>
get_triangles(const landscape &land, size_t start_offset) noexcept {
  std::array<glm::vec3, vertixies_in_triangle * N> triangles_set;
  const auto indexes = std::span<const GLuint>{land.indexes()}.subspan(
      start_offset, vertixies_in_triangle * N);
  std::transform(
      indexes.begin(), indexes.end(), triangles_set.begin(),
      [vert = land.verticies()](GLuint idx) { return vert[idx].position; });
  return triangles_set;
}

struct box {
  glm::vec3 min{std::numeric_limits<float>::max()};
  glm::vec3 max{std::numeric_limits<float>::min()};

  [[nodiscard]] float length() const noexcept {
    return max.x > min.x ? max.x - min.x : 0.f;
  }
  [[nodiscard]] float width() const noexcept {
    return max.y > min.y ? max.y - min.y : 0.f;
  }
  [[nodiscard]] float height() const noexcept {
    return max.z > min.z ? max.z - min.z : 0.f;
  }
};

box expand(box b, vertex pt) noexcept {
  b.min.x = std::min(b.min.x, pt.position.x);
  b.min.y = std::min(b.min.y, pt.position.y);
  b.min.z = std::min(b.min.z, pt.position.z);

  b.max.x = std::max(b.max.x, pt.position.x);
  b.max.y = std::max(b.max.y, pt.position.y);
  b.max.z = std::max(b.max.z, pt.position.z);
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
  const meters radius = centimeters{10} * GENERATE(range(9, 11));
  INFO(fmt::format("{}x{} hexagons of size {}", columns, rows, radius.count()));
  landscape land{radius, columns, rows};

  SECTION("covers flat rectangular area") {
    std::span<const vertex> vert = land.verticies();
    const box landscape_bounds = bounding_box(vert.begin(), vert.end());
    SECTION("mesh is flat") {
      CHECK_THAT(landscape_bounds.height(),
                 Catch::Matchers::WithinAbs(0., 0.0001));
    }
    SECTION("mesh has proper x dimensions") {
      CHECK(landscape_bounds.length() ==
            Approx((2 + (1 + std::cos(M_PI / 3)) * (columns - 1)) *
                   radius.count()));
    }
    SECTION("mesh has proper y dimensions") {
      CHECK(landscape_bounds.width() ==
            Approx(rows * 2 * radius.count() * std::sin(M_PI / 3)));
    }
  }

  SECTION("generates set of hexagons") {

    REQUIRE(land.indexes().size() % vertixies_in_hexagon == 0);

    for (int index_set_start = 0; index_set_start < land.indexes().size();
         index_set_start += vertixies_in_hexagon) {
      std::array<glm::vec3, vertixies_in_hexagon> triangles_set =
          get_triangles<triangles_in_hexagon>(land, index_set_start);

      SECTION(fmt::format(
          "triangles {} - {} of {} forms hexagon", index_set_start,
          index_set_start + vertixies_in_hexagon, land.indexes().size())) {
        for (size_t triangle_offset = 0; triangle_offset < triangles_set.size();
             triangle_offset += vertixies_in_triangle) {
          std::span<const glm::vec3, vertixies_in_triangle> triangle{
              triangles_set.data() + triangle_offset, vertixies_in_triangle};

          SECTION(fmt::format("triangle {} is equilateral",
                              triangle_offset / triangles_in_hexagon)) {
            CHECK(glm::dot(triangle[1] - triangle[0],
                           triangle[2] - triangle[0]) ==
                  Approx(0.5 * radius.count() * radius.count()));
            CHECK(glm::dot(triangle[0] - triangle[1],
                           triangle[2] - triangle[1]) ==
                  Approx(0.5 * radius.count() * radius.count()));
            CHECK(glm::dot(triangle[0] - triangle[2],
                           triangle[1] - triangle[2]) ==
                  Approx(0.5 * radius.count() * radius.count()));
          }

          SECTION("there are 7 unique points (center and hexagon perimeter)") {
            std::sort(triangles_set.begin(), triangles_set.end(),
                      [](glm::vec3 l, glm::vec3 r) {
                        return std::tie(l.x, l.y, l.z) <
                               std::tie(r.x, r.y, r.z);
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
