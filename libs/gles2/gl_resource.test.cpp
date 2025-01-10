#include <algorithm>
#include <ranges>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>

#include <libs/gles2/resource.hpp>

using namespace Catch::Matchers;

std::vector<GLuint> deleted_handles;

enum class test_type { scalar, array };

using scalar_handle = gl::basic_handle<test_type::scalar>;
template <>
inline void scalar_handle::free(scalar_handle res) noexcept {
  deleted_handles.push_back(res.handle_);
}
using scalar_resource = gl::resource<scalar_handle>;

using array_handle = gl::basic_handle<test_type::array>;
template <>
inline void array_handle::free(std::span<array_handle> res) noexcept {
  std::ranges::copy(
      res | std::views::transform([](array_handle h) { return h.native_handle(); }),
      std::back_inserter(deleted_handles)
  );
}
template <size_t N>
using array_resource = gl::resource<array_handle[N]>;

TEST_CASE("scalar gl::resource") {
  deleted_handles.clear();

  SECTION("free must not be called on default constructed resource") {
    { scalar_resource res; }
    REQUIRE_THAT(deleted_handles, IsEmpty());
  }
  SECTION("free must be called once on nonzero resource") {
    { scalar_resource res{scalar_handle{5}}; }
    REQUIRE(deleted_handles == std::vector<GLuint>{5});
  }
  SECTION("free must not be called on object moved to constructor") {
    scalar_resource owned = [&] {
      scalar_resource src{scalar_handle{5}};
      return scalar_resource{std::move(src)};
    }();
    REQUIRE_THAT(deleted_handles, IsEmpty());
  }
  SECTION("free must not be called on object moved to asignment") {
    scalar_resource res;
    {
      scalar_resource tmp{scalar_handle{5}};
      res = std::move(tmp);
    }
    REQUIRE_THAT(deleted_handles, IsEmpty());
  }
  SECTION("free must be called on nonzero object assigned with new value") {
    scalar_resource res{scalar_handle{42}};
    {
      scalar_resource tmp{scalar_handle{5}};
      res = std::move(tmp);
    }
    REQUIRE(deleted_handles == std::vector<GLuint>{42});
  }
}

TEST_CASE("array gl::resource") {
  deleted_handles.clear();

  SECTION("free must not be called on default constructed resource") {
    { array_resource<2> res; }
    REQUIRE_THAT(deleted_handles, IsEmpty());
  }

  std::array<array_handle, 2> handls{array_handle{5}, array_handle{6}};
  SECTION("free must be called once on nonzero resource") {
    { array_resource<2> res{handls}; }
    REQUIRE(deleted_handles == std::vector<GLuint>{5, 6});
  }
  SECTION("free must not be called on object moved to constructor") {
    auto owned = [&] {
      array_resource<2> src{handls};
      return array_resource<2>{std::move(src)};
    }();
    REQUIRE_THAT(deleted_handles, IsEmpty());
  }
  SECTION("free must not be called on object moved to asignment") {
    array_resource<2> res;
    {
      array_resource<2> tmp{handls};
      res = std::move(tmp);
    }
    REQUIRE_THAT(deleted_handles, IsEmpty());
  }
  SECTION("free must be called on nonzero object assigned with new value") {
    std::array<array_handle, 2> old_handls{array_handle{7}, array_handle{8}};
    array_resource<2> res{old_handls};
    {
      array_resource<2> tmp{handls};
      res = std::move(tmp);
    }
    REQUIRE(deleted_handles == std::vector<GLuint>{7, 8});
  }
}

static_assert(gl::resource_handle<scalar_handle>);
static_assert(gl::scalar_resource_handle<scalar_handle>);
static_assert(gl::resource_handle<array_handle>);
static_assert(gl::array_resource_handle<array_handle>);
