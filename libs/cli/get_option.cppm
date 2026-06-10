module;

#include <algorithm>
#include <cassert>
#include <span>
#include <string_view>

export module cli:get_option;

export
bool get_flag(std::span<char*>& args, std::string_view flag) noexcept {
  assert(!args.empty());
  auto first = std::next(args.begin());
  auto last = args.end();
  auto fres = std::remove(first, last, flag);
  args = args.subspan(0, args.size() - std::distance(fres, last));
  return fres != last;
}

export
const char* get_option(std::span<char*>& args, std::string_view option) noexcept {
  assert(!args.empty());
  auto first = std::next(args.begin());
  auto last = args.end();
  auto fres = std::adjacent_find(first, last, [&](const char* opt, const char* val) {
    return opt == option && val[0] != '-';
  });
  if (fres == last)
    return nullptr;
  fres = std::rotate(fres, std::next(fres, 2), last);
  args = args.subspan(0, args.size() - 2);
  return *std::next(fres);
}

export
template <typename T>
    std::decay_t<T> get_option(std::span<char*>& args, std::string_view option, T&& default_val) noexcept {
  const char* val = get_option(args, option);
  if (!val)
    return std::forward<T>(default_val);
  return std::decay_t<T>{val};
}
