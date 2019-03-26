#include <string_view>

#include <gsl/string_span>

bool has_flag(int argc, char** argv, std::string_view flag) noexcept;
gsl::czstring<> get_option(int argc, char** argv, std::string_view option) noexcept;

template<typename T>
std::decay_t<T> get_option(int argc, char** argv, std::string_view option, T&& default_val) noexcept {
  gsl::czstring<> val = get_option(argc, argv, option);
  if (!val)
    return std::forward<T>(default_val);
  return std::decay_t<T>{val};
}
