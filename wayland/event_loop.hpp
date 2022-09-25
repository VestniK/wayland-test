#pragma once

#include <system_error>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <wayland/wayland-client.hpp>

template <typename Service> struct identified {
  wl::unique_ptr<Service> service;
  uint32_t id = {};
};

class event_loop {
public:
  event_loop(const char *display);

  [[nodiscard]] wl_display &get_display() const noexcept { return *display_; }
  [[nodiscard]] wl_compositor *get_compositor() const noexcept {
    return compositor_.service.get();
  }
  [[nodiscard]] ivi_application *get_ivi() const noexcept {
    return ivi_.service.get();
  }
  [[nodiscard]] xdg_wm_base *get_xdg_wm() const noexcept {
    return xdg_wm_.service.get();
  }

  void dispatch(std::error_code &ec) noexcept;
  void dispatch();
  void dispatch_pending(std::error_code &ec) noexcept;
  void dispatch_pending();

  asio::awaitable<void> dispatch(asio::io_context::executor_type exec);

private:
  std::error_code check() noexcept;

  static void global(void *data, wl_registry *reg, uint32_t id,
                     const char *name, uint32_t ver);
  static void global_remove(void *data, wl_registry *, uint32_t id);

private:
  wl::unique_ptr<wl_display> display_;
  wl::unique_ptr<wl_registry> registry_;
  wl_registry_listener listener_ = {&global, &global_remove};
  identified<wl_compositor> compositor_;
  identified<ivi_application> ivi_;
  identified<xdg_wm_base> xdg_wm_;
};
