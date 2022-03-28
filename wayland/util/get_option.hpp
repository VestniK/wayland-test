#include <span>
#include <string_view>

bool get_flag(std::span<char *> &args, std::string_view flag) noexcept;
const char *get_option(std::span<char *> &args,
                       std::string_view option) noexcept;

template <typename T>
std::decay_t<T> get_option(std::span<char *> &args, std::string_view option,
                           T &&default_val) noexcept {
  const char *val = get_option(args, option);
  if (!val)
    return std::forward<T>(default_val);
  return std::decay_t<T>{val};
}
