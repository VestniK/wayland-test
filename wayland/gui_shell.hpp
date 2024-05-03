#pragma once

#include <libs/geom/geom.hpp>

#include <wayland/event_loop.hpp>
#include <wayland/ivi_window.hpp>
#include <wayland/shell_window.hpp>
#include <wayland/xdg_window.hpp>

namespace wl {

class gui_shell {
public:
  gui_shell(event_loop& eloop);

  gui_shell(const gui_shell&) = delete;
  gui_shell& operator=(const gui_shell&) = delete;
  gui_shell(gui_shell&&) = delete;
  gui_shell& operator=(gui_shell&&) = delete;
  ~gui_shell() noexcept = default;

  [[nodiscard]] wl_compositor* get_compositor() const noexcept {
    return compositor_.service.get();
  }
  [[nodiscard]] ivi_application* get_ivi() const noexcept {
    return ivi_.service.get();
  }
  [[nodiscard]] xdg_wm_base* get_xdg_wm() const noexcept {
    return xdg_wm_.service.get();
  }

  std::error_code check() noexcept;

  asio::awaitable<sized_window<shell_window>> create_maximized_window(
      event_loop& eloop, asio::io_context::executor_type io_exec);

private:
  static void global(void* data, wl_registry* reg, uint32_t id,
      const char* name, uint32_t ver);
  static void global_remove(void* data, wl_registry*, uint32_t id);

private:
  wl::unique_ptr<wl_registry> registry_;
  wl_registry_listener listener_ = {&global, &global_remove};
  identified<wl_compositor> compositor_;
  identified<ivi_application> ivi_;
  identified<xdg_wm_base> xdg_wm_;
  xdg_wm_base_listener xdg_listener_;
};

} // namespace wl
