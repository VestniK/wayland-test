#include <cstdlib>
#include <stdexcept>

#include "xdg.h"

namespace xdg {

namespace {

using czstring = const char *;

bool empty(czstring str) noexcept {
  return !str || str[0] == '\0';
}

}

fs::path runtime_dir() {
  czstring env = ::getenv("XDG_RUNTIME_DIR");
  if (empty(env))
    throw std::runtime_error{"environment variable ${XDG_RUNTIME_DIR} is not set"};
  return env;
}

} // namespace xdg
