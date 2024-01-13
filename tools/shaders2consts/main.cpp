#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string_view>
#include <system_error>
#include <vector>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <util/struct_args.hpp>

#include "parse_include.hpp"
#include "split_shaders.hpp"

namespace fs = std::filesystem;
using namespace std::literals;

struct opts {
  std::vector<fs::path> input = args::option<std::vector<fs::path>>(
      "-i", "--input", "input GLSL shader file to process");
  fs::path output = args::option<fs::path>(
      "-o", "--output", "output C++ source with string constans");
  fs::path header = args::option<fs::path>(
      "--header", "output C++ header with shader constants");
  fs::path incdir = args::option<fs::path>("-I", "--include-dir",
      "path to a directory where to search files from @include statement");
  std::optional<fs::path> depfile =
      args::option<std::optional<fs::path>>("-d", "--depfile",
          "output depfile to inform build system about @include dependencies");
};

std::ifstream ifopen(const fs::path& path) {
  std::ifstream in{path};
  if (!in)
    throw std::system_error{errno, std::system_category(),
        fmt::format("std::ifstream{{{}}}", path.string())};
  return in;
}

std::ofstream ofopen(const fs::path& path) {
  std::ofstream out{path};
  if (!out)
    throw std::system_error{errno, std::system_category(),
        fmt::format("std::ofstream{{{}}}", path.string())};
  return out;
}

void include_content(std::ostream& out, const fs::path& file) {
  auto in = ifopen(file);
  std::copy(std::istreambuf_iterator{in.rdbuf()}, {},
      std::ostreambuf_iterator{out.rdbuf()});
  out << '\n';
}

std::string filename_without_extensions(const fs::path& path) {
  auto res = path.filename();
  while (res.has_extension())
    res.replace_extension();
  return res;
}

int main(int argc, char** argv) {
  std::span<char*> args{argv, static_cast<size_t>(argc)};
  if (get_flag(args, "-h")) {
    args::usage<opts>(args.front(), std::cout);
    args::args_help<opts>(std::cout);
    return 0;
  }
  const auto opts = args::parse<::opts>(args);

  auto depfile = opts.depfile.transform([&](const fs::path& p) {
    auto res = ofopen(p);
    res << opts.output.string() << ':';
    return res;
  });
  std::ofstream out = ofopen(opts.output);

  auto resolver = [&](const std::filesystem::path& inc) {
    const auto resolved = opts.incdir / inc;
    if (depfile)
      depfile.value() << ' ' << fs::absolute(resolved).string();
    return ifopen(resolved);
  };
  shader_pieces_builder builder;

  std::map<std::string, std::vector<size_t>> inputs;
  for (const auto& input : opts.input) {
    std::ifstream in = ifopen(input);
    inputs[filename_without_extensions(input)] =
        builder.parse_shader(in, resolver);
  }
  const auto strings = std::move(builder).finish();

  out << "#include <span>\n\n";
  out << "namespace shaders {\n\n";
  out << "namespace {\n";

  fmt::print(out, R"FMT(
constexpr const char* shader_strings[] = {{
  {}
}};
)FMT",
      fmt::join(strings | std::views::transform([](auto&& v) {
        return fmt::format("R\"GLSL({})GLSL\"", v);
      }),
          ",\n\n  "));

  for (const auto& [name, idxs] : inputs) {
    fmt::print(out, R"FMT(
constexpr const char* {}_arr[] = {{{}}};
)FMT",
        name,
        fmt::join(idxs | std::views::transform([](size_t i) {
          return fmt::format("shader_strings[{}]", i);
        }),
            ", "));
  }

  out << "\n} // namespace\n\n";

  for (const auto& [name, _] : inputs) {
    fmt::print(out, "std::span<const char* const> {0}{{{0}_arr}};\n", name);
  }

  out << "\n} // namespace shaders\n";

  out = ofopen(opts.header);
  out << "#include <span>\n\n";
  out << "namespace shaders {\n\n";
  for (const auto& [name, _] : inputs) {
    fmt::print(out, "extern std::span<const char* const> {0};\n", name);
  }
  out << "\n} // namespace shaders\n";

  if (depfile)
    depfile.value() << '\n';

  return 0;
}
