#include <array>

#include <gsl/span>

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <wayland/gles2/landscape.hpp>

using namespace fmt::literals;
using namespace Catch::literals;

namespace test {

namespace {

constexpr unsigned vertixies_in_triangle = 3;
constexpr unsigned triangles_in_hexagon = 6;
constexpr unsigned vertixies_in_hexagon =
    vertixies_in_triangle * triangles_in_hexagon;

template <size_t N>
std::array<glm::vec3, vertixies_in_triangle * N> get_triangles(
    const mesh_data& mesh, size_t start_offset) noexcept {
  std::array<glm::vec3, vertixies_in_triangle * N> triangles_set;
  const auto indexes = gsl::span<const GLuint>{mesh.indexes}.subspan(
      start_offset, vertixies_in_triangle * N);
  std::transform(indexes.begin(), indexes.end(), triangles_set.begin(),
      [&](GLuint idx) { return mesh.verticies[idx].position; });
  return triangles_set;
}

} // namespace

TEST_CASE("landscape consists of hexagons") {
  const mesh_data landscape = generate_flat_landscape(1.0, 5, 3);

  REQUIRE(landscape.indexes.size() % vertixies_in_hexagon == 0);

  for (size_t index_set_start = 0; index_set_start < landscape.indexes.size();
       index_set_start += vertixies_in_hexagon) {
    std::array<glm::vec3, vertixies_in_hexagon> triangles_set =
        get_triangles<triangles_in_hexagon>(landscape, index_set_start);

    SECTION("triangles {} - {} of {} forms hexagon"_format(index_set_start,
        index_set_start + vertixies_in_hexagon, landscape.indexes.size())) {
      for (size_t triangle_offset = 0; triangle_offset < triangles_set.size();
           triangle_offset += vertixies_in_triangle) {
        gsl::span<const glm::vec3, vertixies_in_triangle> triangle{
            triangles_set.data() + triangle_offset, vertixies_in_triangle};

        SECTION("triangle {} is equilateral"_format(
            triangle_offset / triangles_in_hexagon)) {
          CHECK(glm::dot(triangle[1] - triangle[0],
                    triangle[2] - triangle[0]) == 0.5_a);
          CHECK(glm::dot(triangle[0] - triangle[1],
                    triangle[2] - triangle[1]) == 0.5_a);
          CHECK(glm::dot(triangle[0] - triangle[2],
                    triangle[1] - triangle[2]) == 0.5_a);
        }

        SECTION("there are 7 unique points (center and hexagon perimeter)") {
          std::sort(triangles_set.begin(), triangles_set.end(),
              [](glm::vec3 l, glm::vec3 r) {
                return std::tie(l.x, l.y, l.z) < std::tie(r.x, r.y, r.z);
              });
          auto it = std::unique(triangles_set.begin(), triangles_set.end());
          CHECK(std::distance(triangles_set.begin(), it) == 7);
        }
      }
    }
  }
}

} // namespace test
