#pragma once

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace xdg {

fs::path runtime_dir() {
  using czstring = const char*;
  czstring env = ::getenv("XDG_RUNTIME_DIR");
  if (!env || *env == 0)
    throw std::runtime_error{"environment variable ${XDG_RUNTIME_DIR} is not set"};
  return env;
}

} // namespace xdg
