#pragma once

#include <wayland/util/geom.hpp>
#include <wayland/wlutil.hpp>

struct wl_compositor;
struct wl_surface;
struct ivi_application;

namespace ivi {

class window {
public:
  window() = delete;
  window(const window &) = delete;
  window &operator=(const window &) = delete;
  window(window &&rhs) = delete;
  window &operator=(window &&rhs) = delete;

  virtual ~window() noexcept = default;

  window(wl_compositor &compositor, ivi_application &shell, uint32_t ivi_id);

  wl_surface &get_surface() { return *surface_; }

protected:
  virtual void resize(size sz) = 0;

private:
  static void configure(void *data, ivi_surface *ivi_surface, int32_t width,
                        int32_t height);
  static ivi_surface_listener ivi_listener;

private:
  wl::unique_ptr<wl_surface> surface_;
  wl::unique_ptr<ivi_surface> ivi_surface_;
};

} // namespace ivi
