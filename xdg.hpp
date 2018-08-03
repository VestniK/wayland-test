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

fs::path config_home() {
  using czstring = const char*;
  czstring env = ::getenv("XDG_CONFIG_HOME");
  if (env && *env != 0)
    return env;

  env = ::getenv("HOME");
  if (!env || *env == 0)
    throw std::runtime_error{"environment variable ${HOME} is not set"};
  return fs::path{env}/".config";
}

fs::path find_config(fs::path relative) {
  using czstring = const char*;

  if (relative.is_absolute())
    return relative;

  const fs::path user_cfg = config_home()/relative;
  if (fs::exists(user_cfg))
    return user_cfg;

  czstring env = ::getenv("XDG_CONFIG_DIRS");
  if (!env || *env == 0)
    env = "/etc/xdg";

  for (const char* p = env; *p != 0; ++p) {
    if (*p != ':')
      continue;
    const auto candidate = fs::path{env, p}/relative;
    if (fs::exists(candidate))
      return candidate;
    env = p + 1;
  }

  return {};
}

} // namespace xdg
