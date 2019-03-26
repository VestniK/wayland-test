#pragma once

#include <system_error>

enum class ui_errc {
  window_closed = 1,
  compositor_lost,
  shell_lost
};

const std::error_category& ui_category() noexcept;

inline std::error_code make_error_code(ui_errc err) noexcept {
  return {static_cast<int>(err), ui_category()};
}

namespace std {
template<> struct is_error_code_enum<ui_errc>: std::true_type {};
}
