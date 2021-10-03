#include <algorithm>
#include <cassert>

#include <wayland/util/get_option.hpp>

bool get_flag(std::span<char *> &args, std::string_view flag) noexcept {
  assert(!args.empty());
  auto first = std::next(args.begin());
  auto last = args.end();
  auto fres = std::remove(first, last, flag);
  args = args.subspan(0, args.size() - std::distance(fres, last));
  return fres != last;
}

gsl::czstring<> get_option(std::span<char *> &args,
                           std::string_view option) noexcept {
  assert(!args.empty());
  auto first = std::next(args.begin());
  auto last = args.end();
  auto fres = std::adjacent_find(first, last,
                                 [&](gsl::czstring<> opt, gsl::czstring<> val) {
                                   return opt == option && val[0] != '-';
                                 });
  if (fres == last)
    return nullptr;
  fres = std::rotate(fres, std::next(fres, 2), last);
  args = args.subspan(0, args.size() - 2);
  return *std::next(fres);
}
