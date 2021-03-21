#include <utility>

#include <ivi-application.h>

#include <wayland/window.hpp>
#include <wayland/wlutil.hpp>

namespace ivi {

window::window(wl_compositor &compositor, ivi_application &shell,
               uint32_t ivi_id)
    : surface_{wl::unique_ptr<wl_surface>{
          wl_compositor_create_surface(&compositor)}},
      ivi_surface_{wl::unique_ptr<ivi_surface>{
          ivi_application_surface_create(&shell, ivi_id, surface_.get())}} {
  ivi_surface_add_listener(ivi_surface_.get(), &ivi_listener, this);
}

ivi_surface_listener window::ivi_listener = {
    .configure = window::configure,
};

void window::configure(void *data, ivi_surface *, int32_t width,
                       int32_t height) {
  auto *self = reinterpret_cast<window *>(data);
  self->resize({width, height});
}

} // namespace ivi
