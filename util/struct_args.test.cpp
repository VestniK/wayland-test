#include <util/struct_args.hpp>

#include <catch2/catch_test_macros.hpp>

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
      std::string_view name =
          args::option<std::string_view>{"--name", "User name"};
      std::string_view threads = args::option<std::string_view>{
          "-j", "--threads", "Number of threads to use"};
    };

    WHEN("args with all required options are parsed") {
      cli_args argv{{"--name", "Vasja"}};
      auto opt = args::parse<opts>(argv);

      THEN("name value is parsed") { REQUIRE(opt.name == "Vasja"); }
    }

    WHEN("args help is requestd") {
      std::ostringstream out;
      args::args_help<opts>(out);

      THEN("all members are documented") {
        REQUIRE(out.str() == "\t--name VAL\tUser name\n"
                             "\t--threads, -j VAL\tNumber of threads to use\n");
      }
    }

    WHEN("usage is requested") {
      std::ostringstream out;
      args::usage<opts>("prog_name", out);

      THEN("all members are listed") {
        REQUIRE(out.str() == "Usage: prog_name --name VAL -j VAL\n");
      }
    }
  }
}
