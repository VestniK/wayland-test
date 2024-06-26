#include <libs/wlwnd/ui_category.hpp>

const std::error_category& ui_category() noexcept {
  static const struct final : std::error_category {
    [[nodiscard]] const char* name() const noexcept override { return "UI"; }

    [[nodiscard]] std::string message(int cond) const override {
      if (cond == 0)
        return std::generic_category().message(0);
      switch (static_cast<ui_errc>(cond)) {
      case ui_errc::window_closed:
        return "Window is closed";
      case ui_errc::compositor_lost:
        return "Wayland compositor is not available";
      case ui_errc::shell_lost:
        return "Wayland shell is not available";
      }
      return "Unknown UI error";
    }
  } inst;
  return inst;
}
