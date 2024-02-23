#pragma once

#include <wayland/window_delegate.hpp>
#include <wayland/wlutil.hpp>

struct wl_compositor;
struct wl_surface;
struct ivi_application;

namespace ivi {

using delegate = basic_delegate;

class window {
public:
  window(const window&) = delete;
  window& operator=(const window&) = delete;

  window() noexcept = default;
  window(window&&) noexcept = default;
  window& operator=(window&& rhs) noexcept = default;

  window(wl_compositor& compositor, ivi_application& shell, uint32_t ivi_id,
      delegate* delegate = nullptr);

  wl_surface& get_surface() { return *surface_; }
  ivi_surface& get_ivi_surface() noexcept { return *ivi_surface_; }
  void set_delegate(delegate* delegate) noexcept;
  void set_delegate_queue(wl_event_queue& queue) noexcept;

private:
  static void configure(
      void* data, ivi_surface* ivi_surface, int32_t width, int32_t height);
  static ivi_surface_listener ivi_listener;

private:
  wl::unique_ptr<wl_surface> surface_;
  wl::unique_ptr<ivi_surface> ivi_surface_;
};

} // namespace ivi
