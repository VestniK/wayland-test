#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <string_view>
#include <vector>

#include <wayland/get_option.hpp>

namespace test {

using namespace std::literals;

TEST_CASE("get_option") {
  char arg0[] = "prog";
  char arg1[] = "-d";
  char arg2[] = "wayland-1";
  std::vector<char*> args = {arg0, arg1, arg2};
  char** argv = args.data();
  int argc = static_cast<int>(args.size());

  SECTION("returns nullptr when there is no requested option") {
    REQUIRE(get_option(argc, argv, "-s") == nullptr);
  }
  SECTION("returns requested option value") {
    REQUIRE(get_option(argc, argv, "-d") == "wayland-1"sv);
  }
  SECTION("removes found option") {
    get_option(argc, argv, "-d");
    REQUIRE(argc == 1);
    REQUIRE(argv[0] == "prog"sv);
  }
}

} // namespace test
