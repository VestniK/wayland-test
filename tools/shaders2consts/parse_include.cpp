#include <string_view>

#include "parse_include.hpp"

namespace {

constexpr std::string_view ltrim(std::string_view in) noexcept {
  const char* it = in.begin();
  while (it != in.end() && (*it == ' ' || *it == '\t'))
    ++it;
  return std::string_view{it, in.end()};
}

constexpr std::string_view rtrim(std::string_view in) noexcept {
  auto it = in.rbegin();
  while (it != in.rend() && (*it == ' ' || *it == '\t'))
    ++it;
  return std::string_view{in.begin(), it.base()};
}

} // namespace

std::expected<std::filesystem::path, include_parse_error> parse_include(
    std::string_view ln) {
  using enum include_parse_error;
  constexpr std::string_view include = "@include";

  ln = ltrim(ln);

  if (!ln.starts_with(include))
    return std::unexpected(non_include_line);
  ln = ltrim(ln.substr(include.size()));

  if (!ln.starts_with('"'))
    return std::unexpected(missing_open_quot);
  ln.remove_prefix(1);

  ln = rtrim(ln);
  if (!ln.ends_with('"'))
    return std::unexpected(missing_close_quot);
  ln.remove_suffix(1);

  return std::filesystem::path{ln};
}
