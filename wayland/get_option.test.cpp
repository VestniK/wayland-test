#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <string_view>
#include <vector>

#include <wayland/get_option.hpp>

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
  std::vector<char*> args = {arg0, arg1, arg2, arg3, arg4, arg5, arg6};
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
    REQUIRE(arg_views{argv, argv + argc} ==
            arg_views{"prog", "-m", "nsk", "-m", "msk"});
  }
  SECTION("returns repeating options preserving order") {
    arg_views maps;
    while (const char* map = get_option(argc, argv, "-m"))
      maps.push_back(map);
    REQUIRE(maps == arg_views{{"nsk", "msk"}});
  }
}

} // namespace test
