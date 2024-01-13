#pragma once

#include <expected>
#include <filesystem>

enum class include_parse_error {
  non_include_line,
  missing_open_quot,
  missing_close_quot
};

std::expected<std::filesystem::path, include_parse_error> parse_include(
    std::string_view ln);
