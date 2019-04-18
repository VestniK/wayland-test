#include <algorithm>
#include <cassert>

#include <wayland/util/get_option.hpp>

bool get_flag(int& argc, char** argv, std::string_view flag) noexcept {
  assert(argc > 0);
  char** first = argv + 1;
  char** last = argv + argc;
  char** fres = std::remove(first, last, flag);
  argc -= std::distance(fres, last);
  return fres != last;
}

gsl::czstring<> get_option(
    int& argc, char** argv, std::string_view option) noexcept {
  assert(argc > 0);
  char** first = argv + 1;
  char** last = argv + argc;
  char** fres = std::adjacent_find(
      first, last, [&](gsl::czstring<> opt, gsl::czstring<> val) {
        return opt == option && val[0] != '-';
      });
  if (fres == last)
    return nullptr;
  fres = std::rotate(fres, std::next(fres, 2), last);
  argc -= 2;
  return *std::next(fres);
}
