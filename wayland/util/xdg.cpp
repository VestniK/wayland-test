#include <cstdlib>

#include <gsl/string_span>

#include <wayland/util/xdg.hpp>

namespace fs = std::filesystem;

namespace xdg {

fs::path config_home() {
  gsl::czstring<> env = ::getenv("XDG_CONFIG_HOME");
  if (env && *env != 0)
    return env;

  env = ::getenv("HOME");
  if (!env || *env == 0)
    throw std::runtime_error{"environment variable ${HOME} is not set"};
  return fs::path{env} / ".config";
}

fs::path find_config(fs::path relative) {
  if (relative.is_absolute())
    return relative;

  const fs::path user_cfg = config_home() / relative;
  if (fs::exists(user_cfg))
    return user_cfg;

  gsl::czstring<> env = ::getenv("XDG_CONFIG_DIRS");
  if (!env || *env == 0)
    env = "/etc/xdg";

  for (const char* p = env; *p != 0; ++p) {
    if (*p != ':')
      continue;
    const auto candidate = fs::path{env, p} / relative;
    if (fs::exists(candidate))
      return candidate;
    env = p + 1;
  }

  return {};
}

// TODO: уменьшить копипасту
fs::path data_home() {
  gsl::czstring<> env = ::getenv("XDG_DATA_HOME");
  if (env && *env != 0)
    return env;

  env = ::getenv("HOME");
  if (!env || *env == 0)
    throw std::runtime_error{"environment variable ${HOME} is not set"};
  return fs::path{env} / ".local/share";
}

// TODO: уменьшить копипасту
fs::path find_data(fs::path relative) {
  if (relative.is_absolute())
    return relative;

  const fs::path user_cfg = data_home() / relative;
  if (fs::exists(user_cfg))
    return user_cfg;

  gsl::czstring<> env = ::getenv("XDG_DATA_DIRS");
  if (!env || *env == 0)
    env = "/usr/local/share/:/usr/share/";

  for (const char* p = env; *p != 0; ++p) {
    if (*p != ':')
      continue;
    const auto candidate = fs::path{env, p} / relative;
    if (fs::exists(candidate))
      return candidate;
    env = p + 1;
  }

  return {};
}

} // namespace xdg
