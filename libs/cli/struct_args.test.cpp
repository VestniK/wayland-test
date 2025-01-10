#include <libs/cli/struct_args.hpp>

#include "catch2/matchers/catch_matchers_container_properties.hpp"
#include "catch2/matchers/catch_matchers_contains.hpp"
#include "catch2/matchers/catch_matchers_range_equals.hpp"
#include <catch2/catch_test_macros.hpp>

using Catch::Matchers::Contains;
using Catch::Matchers::IsEmpty;
using Catch::Matchers::RangeEquals;
using Catch::Matchers::SizeIs;

namespace {

class cli_args {
public:
  cli_args(std::initializer_list<const char*> args) {
    arg_strings_.push_back("prog_name");
    for (const char* arg : args)
      arg_strings_.push_back(arg);
    for (auto& arg : arg_strings_)
      args_.push_back(arg.data());
  }

  operator std::span<char*>() noexcept { return args_; }

private:
  std::vector<std::string> arg_strings_;
  std::vector<char*> args_;
};

} // namespace

SCENARIO("parse CLI options") {
  GIVEN("opts type") {
    struct opts {
      std::string_view name = args::option<std::string_view>{"--name", "User name"};
      std::string_view threads =
          args::option<std::string_view>{"-j", "--threads", "Number of threads to use"}.default_value("8");
      std::vector<std::string_view> incdirs =
          args::option<std::vector<std::string_view>>("-I", "--include", "include directories");
    };

    WHEN("args with all required options are parsed") {
      cli_args argv{{"--name", "Vasja"}};
      auto opt = args::parse<opts>(argv);

      THEN("name value is parsed") { REQUIRE(opt.name == "Vasja"); }

      THEN("threads is set to default value") { REQUIRE(opt.threads == "8"); }

      THEN("include dirs list is empty") { REQUIRE_THAT(opt.incdirs, IsEmpty()); }
    }

    WHEN("optional arg is passed") {
      cli_args argv{{"--name", "Vasja", "-j", "32"}};
      auto opt = args::parse<opts>(argv);

      THEN("threads is set to specified falue") { REQUIRE(opt.threads == "32"); }
    }

    WHEN("collection option is specified once") {
      cli_args argv{{"--name", "Vasja", "-I", "include/name"}};
      auto opt = args::parse<opts>(argv);

      THEN("incdirs contains passed dir") {
        REQUIRE_THAT(opt.incdirs, RangeEquals(std::vector{"include/name"}));
      }
    }

    WHEN("collection option is specified multile times") {
      cli_args argv{{"--name", "Vasja", "-I", "include/name", "-I", "build/include/name"}};
      auto opt = args::parse<opts>(argv);

      THEN("incdirs contains passed dir") {
        REQUIRE_THAT(opt.incdirs, RangeEquals(std::vector{"include/name", "build/include/name"}));
      }
    }

    WHEN("args help is requestd") {
      std::ostringstream out;
      args::args_help<opts>(out);

      THEN("all members are documented") {
        REQUIRE(
            out.str() == "\t--name VAL\tUser name\n"
                         "\t--threads, -j VAL\tNumber of threads to use\n"
                         "\t--include, -I VAL\tinclude directories\n"
        );
      }
    }

    WHEN("usage is requested") {
      std::ostringstream out;
      args::usage<opts>("prog_name", out);

      THEN("all members are listed") {
        REQUIRE(out.str() == "Usage: prog_name --name VAL [-j VAL] [-I VAL]\n");
      }
    }
  }
}
