#include <algorithm>
#include <cassert>

#include <wayland/get_option.hpp>

bool has_flag(int argc, char** argv, std::string_view flag) noexcept {
  assert(argc > 0);
  char** first = argv + 1;
  char** last = argv + argc;
  auto it = std::find(first, last, flag);
  return it != last;
}

gsl::czstring<> get_option(int argc, char** argv, std::string_view option) noexcept {
  assert(argc > 0);
  char** first = argv + 1;
  char** last = argv + argc;
  auto it = std::adjacent_find(
    first, last, [&](gsl::czstring<> opt, gsl::czstring<> val) {return opt == option && val[0] !='-';}
  );
  if (it == last)
    return nullptr;
  return *std::next(it);
}
