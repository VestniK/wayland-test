#include <algorithm>
#include <filesystem>
#include <iterator>
#include <ranges>
#include <sstream>
#include <system_error>
#include <unordered_map>

#include <fmt/format.h>

#include "catch2/matchers/catch_matchers_container_properties.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_quantifiers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <testing/matchers/ranges.hpp>

#include "split_shaders.hpp"

using Catch::Generators::from_range;
using Catch::Matchers::IsEmpty;
using Catch::Matchers::NoneMatch;
using Catch::Matchers::RangeEquals;
using namespace std::literals;

struct case_input {
  std::string src;
  std::string inlined;
};

struct case_data {
  std::string name;
  std::vector<case_input> inputs;
};
// clang-format off
const std::vector<case_data> cases{
  {
    .name = "without includes",
    .inputs = {{.src = "some text", .inlined = "some text\n"}}
  },
  {
    .name = "single include at end",
    .inputs = {{
       .src =
R"(some text
@include "foo.glsl")",
       .inlined =
R"(some text
other text
from include
)"
    }}
  },
  {
    .name = "single include at head",
    .inputs = {{
      .src =
R"(@include "foo.glsl"
the end
of text
)",
      .inlined =
R"(other text
from include
the end
of text
)"
    }}
  },
  {
    .name = "single include in the middle",
    .inputs = {{
      .src =
R"(some text
@include "foo.glsl"
the end
of text
)",
      .inlined =
R"(some text
other text
from include
the end
of text
)"
    }}
  },
  {
    .name = "multiple includes",
    .inputs = {{
      .src =
R"(some text
@include "foo.glsl"
@include "bar.glsl"
the end
of text
)",
      .inlined =
R"(some text
other text
from include
step aside
the end
of text
)"
    }}
  },
  {
    .name = "same include twice",
    .inputs = {{
      .src =
R"(head text
@include "bar.glsl"
@include "bar.glsl"
tail text
)",
      .inlined =
R"(head text
step aside
step aside
tail text
)"
    }}
  },
  {
    .name = "nested include",
    .inputs = {{
      .src =
R"(head text
@include "baz.glsl"
tail text
)",
      .inlined =
R"(head text
step aside
and back again
tail text
)"
    }}
  },
  {
    .name = "multiple inputs with common lib",
    .inputs = {{
      .src =
R"(head text
@include "baz.glsl"
tail text
)",
      .inlined =
R"(head text
step aside
and back again
tail text
)"
    },
    {
      .src =
R"(
start and jump into
@include "bar.glsl"
)",
      .inlined =
R"(start and jump into
step aside
)"
    }}
  }
};
// clang-format on

struct {
  std::unordered_map<std::filesystem::path, std::string> files;

  std::stringstream operator()(const std::filesystem::path& path) const {
    const auto it = files.find(path);
    if (it == files.end())
      throw std::system_error{
          std::make_error_code(std::errc::no_such_file_or_directory),
          fmt::format("ifstream{{{}}}", path.string())
      };

    return std::stringstream{it->second};
  }
} test_resolver{
    .files =
        {{"foo.glsl", "other text\nfrom include"},
         {"bar.glsl", "step aside"},
         {"baz.glsl", "@include \"bar.glsl\"\nand back again"}}
};

TEST_CASE("parse shaders into reusable pieces") {
  auto val = GENERATE(from_range(cases));
  shader_pieces_builder builder;

  struct inlined_inputs {
    std::vector<size_t> piece_indexes;
    std::string expected_result;
  };
  std::vector<inlined_inputs> parse_res;
  for (const auto& input : val.inputs) {
    std::stringstream in{input.src};
    parse_res.push_back(
        {.piece_indexes = builder.parse_shader(in, test_resolver), .expected_result = input.inlined}
    );
  }
  const auto pices = std::move(builder).finish();
  SECTION(fmt::format("no pices repeats for '{}'", val.name)) { REQUIRE_THAT(pices, are_unique()); }
  SECTION(fmt::format("no empty pices for '{}'", val.name)) { REQUIRE_THAT(pices, NoneMatch(IsEmpty())); }

  for (const auto& [idx, inl_res] : parse_res | std::views::enumerate) {
    SECTION(fmt::format("check inline result '{}' input {}", val.name, idx)) {
      const auto inlined =
          std::ranges::fold_left(inl_res.piece_indexes, ""s, [&pices](std::string acc, size_t cur) {
            return acc + pices[cur];
          });
      REQUIRE(inlined == inl_res.expected_result);
    }
  }
}
