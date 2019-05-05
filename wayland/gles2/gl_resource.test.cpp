#include <deque>

#include <gsl/pointers>

#include <catch2/catch.hpp>

#include <wayland/gles2/gl_resource.hpp>

namespace test {

std::deque<GLuint> deleted_handles;

struct test_deleter {
  void operator()(GLuint handle) { deleted_handles.push_back(handle); };
};

using test_resource = gl_resource<test_deleter>;

TEST_CASE("gl_resource with stateless deleter") {
  deleted_handles.clear();

  SECTION("deleter must not be called on default constructed resource") {
    { test_resource res; }
    REQUIRE(deleted_handles == std::deque<GLuint>{});
  }
  SECTION("deleter must be called once on nonzero resource") {
    { test_resource res{5}; }
    REQUIRE(deleted_handles == std::deque<GLuint>{5});
  }
  SECTION("deleter must not be called on object moved to constructor") {
    test_resource res{[] { return test_resource{5}; }()};
    REQUIRE(deleted_handles == std::deque<GLuint>{});
  }
  SECTION("deleter must not be called on object moved to asignment") {
    test_resource res;
    {
      test_resource tmp{5};
      res = std::move(tmp);
    }
    REQUIRE(deleted_handles == std::deque<GLuint>{});
  }
  SECTION("deleter must be called on nonzero object assigned with new value") {
    test_resource res{42};
    {
      test_resource tmp{5};
      res = std::move(tmp);
    }
    REQUIRE(deleted_handles == std::deque<GLuint>{42});
  }
}

} // namespace test
