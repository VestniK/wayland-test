#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <vector>

#include <util/get_option.hpp>

namespace test {

using namespace std::literals;

using arg_views = std::vector<std::string_view>;

TEST_CASE("get_option") {
  char arg0[] = "prog";
  char arg1[] = "-m";
  char arg2[] = "nsk";
  char arg3[] = "-d";
  char arg4[] = "wayland-1";
  char arg5[] = "-m";
  char arg6[] = "msk";
  char *args_arr[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6};
  std::span<char *> args = args_arr;

  SECTION("returns nullptr when there is no requested option") {
    REQUIRE(get_option(args, "-s") == nullptr);
  }
  SECTION("returns requested option value") {
    REQUIRE(get_option(args, "-d") == "wayland-1"sv);
  }
  SECTION("removes found option") {
    get_option(args, "-d");
    REQUIRE(arg_views{args.begin(), args.end()} ==
            arg_views{"prog", "-m", "nsk", "-m", "msk"});
  }
  SECTION("returns repeating options preserving order") {
    arg_views maps;
    while (const char *map = get_option(args, "-m"))
      maps.push_back(map);
    REQUIRE(maps == arg_views{{"nsk"sv, "msk"sv}});
  }
}

} // namespace test
