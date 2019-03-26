#pragma once

#include <system_error>

#include <gsl/string_span>

#include <wayland/wayland-client.hpp>

template<typename Service>
struct identified {
  wl::unique_ptr<Service> service;
  uint32_t id = {};
};

class event_loop {
public:
  event_loop(gsl::czstring<> display);

  wl_display& get_display() const noexcept {return *display_;}
  wl_compositor* get_compositor() const noexcept {return compositor.service.get();}
  xdg_wm_base* get_wm() const noexcept {return shell.service.get();}

  void dispatch(std::error_code& ec) noexcept;
  void dispatch();
  void dispatch_pending(std::error_code& ec) noexcept;
  void dispatch_pending();

private:
  std::error_code check() noexcept;

  static void global(void* data, wl_registry* reg, uint32_t id, const char* name, uint32_t ver);
  static void global_remove(void* data, wl_registry*, uint32_t id);

private:
  wl::unique_ptr<wl_display> display_;
  wl::unique_ptr<wl_registry> registry_;
  wl_registry_listener listener_ = {&global, &global_remove};
  identified<wl_compositor> compositor;
  identified<xdg_wm_base> shell;
};
