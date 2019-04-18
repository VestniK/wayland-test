#pragma once

#include <wayland/util/geom.hpp>
#include <wayland/wlutil.hpp>

struct wl_compositor;
struct wl_surface;
struct xdg_wm_base;

class toplevel_window {
public:
  toplevel_window() = delete;
  toplevel_window(const toplevel_window&) = delete;
  toplevel_window& operator=(const toplevel_window&) = delete;
  toplevel_window(toplevel_window&& rhs) = delete;
  toplevel_window& operator=(toplevel_window&& rhs) = delete;

  virtual ~toplevel_window();

  toplevel_window(wl_compositor& compositor, xdg_wm_base& shell);

  wl_surface* get_surface() { return surface_.get(); }

  void maximize() { xdg_toplevel_set_maximized(toplevel_.get()); }

protected:
  virtual void resize(size sz) = 0;
  virtual void close() = 0;
  virtual void display(wl_output& disp) = 0;

private:
  static void configure_surface(void*, xdg_surface* surf, uint32_t serial);
  static void configure_toplevel(
      void* data, xdg_toplevel*, int32_t width, int32_t height, wl_array*);
  static void close(void* data, xdg_toplevel*);
  static void enter_output(void* data, wl_surface*, wl_output* output);
  static void leave_output(void* data, wl_surface*, wl_output* output);

  static xdg_toplevel_listener toplevel_listener;
  static xdg_surface_listener surface_listener;
  static wl_surface_listener surface_output_listener;

private:
  wl::unique_ptr<wl_surface> surface_;
  wl::unique_ptr<xdg_surface> xdg_surface_;
  wl::unique_ptr<xdg_toplevel> toplevel_;
};
