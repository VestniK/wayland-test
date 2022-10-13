#pragma once

#include <wayland/window_delegate.hpp>
#include <wayland/wlutil.hpp>

struct xdg_wm_base;

namespace xdg {

struct delegate : basic_delegate {
  virtual ~delegate() noexcept = default;
  virtual void close() = 0;
  virtual void display(wl_output &disp [[maybe_unused]]){};
};

class toplevel_window {
public:
  toplevel_window(const toplevel_window &) = delete;
  toplevel_window &operator=(const toplevel_window &) = delete;

  toplevel_window() noexcept = default;
  toplevel_window(toplevel_window &&rhs) noexcept = default;
  toplevel_window &operator=(toplevel_window &&rhs) noexcept = default;

  toplevel_window(wl_compositor &compositor, xdg_wm_base &shell,
                  delegate *delegate = nullptr);

  wl_surface &get_surface() { return *surface_; }

  void maximize() { xdg_toplevel_set_maximized(toplevel_.get()); }
  void set_delegate(delegate *delegate) noexcept;
  void set_delegate_queue(wl_event_queue &queue) noexcept;

private:
  static void configure_surface(void *, xdg_surface *surf, uint32_t serial);
  static void configure_toplevel(void *data, xdg_toplevel *, int32_t width,
                                 int32_t height, wl_array *);
  static void close(void *data, xdg_toplevel *);
  static void configure_bounds(void *data, xdg_toplevel *xdg_toplevel,
                               int32_t width, int32_t height);
  static void wm_capabilities(void *data, xdg_toplevel *xdg_toplevel,
                              wl_array *capabilities);
  static void enter_output(void *data, wl_surface *, wl_output *output);
  static void leave_output(void *data, wl_surface *, wl_output *output);

  static xdg_toplevel_listener toplevel_listener;
  static xdg_surface_listener surface_listener;
  static wl_surface_listener surface_output_listener;

private:
  wl::unique_ptr<wl_surface> surface_;
  wl::unique_ptr<xdg_surface> xdg_surface_;
  wl::unique_ptr<xdg_toplevel> toplevel_;
};

} // namespace xdg
