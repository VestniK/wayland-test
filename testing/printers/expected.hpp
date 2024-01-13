#pragma once

#include <expected>
#include <string>

#include <catch2/catch_tostring.hpp>

namespace Catch {
template <typename T, typename E>
struct StringMaker<std::expected<T, E>> {
  static std::string convert(const std::expected<T, E>& val) {
    if (val.has_value())
      return Catch::Detail::stringify(val.value());
    else
      return "unexpected(" + Catch::Detail::stringify(val.error()) + ')';
  }
};
} // namespace Catch
