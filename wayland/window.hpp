#pragma once

#include <wayland/util/geom.hpp>
#include <wayland/wlutil.hpp>

struct wl_compositor;
struct wl_surface;
struct ivi_application;

class ivi_window {
public:
  ivi_window() = delete;
  ivi_window(const ivi_window &) = delete;
  ivi_window &operator=(const ivi_window &) = delete;
  ivi_window(ivi_window &&rhs) = delete;
  ivi_window &operator=(ivi_window &&rhs) = delete;

  virtual ~ivi_window();

  ivi_window(wl_compositor &compositor, ivi_application &shell,
             uint32_t ivi_id);

  wl_surface *get_surface() { return surface_.get(); }

protected:
  virtual void resize(size sz) = 0;
  virtual void close() = 0;
  virtual void display(wl_output &disp) = 0;

private:
  static void configure(void *data, ivi_surface *ivi_surface, int32_t width,
                        int32_t height);
  static void enter_output(void *data, wl_surface *, wl_output *output);
  static void leave_output(void *data, wl_surface *, wl_output *output);

  static ivi_surface_listener ivi_listener;
  static wl_surface_listener surface_output_listener;

private:
  wl::unique_ptr<wl_surface> surface_;
  wl::unique_ptr<ivi_surface> ivi_surface_;
};
