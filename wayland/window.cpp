#include <utility>

#include <ivi-application.h>

#include <wayland/window.hpp>
#include <wayland/wlutil.hpp>

ivi_window::~ivi_window() = default;

ivi_window::ivi_window(wl_compositor &compositor, ivi_application &shell,
                       uint32_t ivi_id)
    : surface_{wl::unique_ptr<wl_surface>{
          wl_compositor_create_surface(&compositor)}},
      ivi_surface_{wl::unique_ptr<ivi_surface>{
          ivi_application_surface_create(&shell, ivi_id, surface_.get())}} {
  ivi_surface_add_listener(ivi_surface_.get(), &ivi_listener, this);
}

wl_surface_listener ivi_window::surface_output_listener = {
    .enter = ivi_window::enter_output,
    .leave = ivi_window::leave_output,
};

ivi_surface_listener ivi_window::ivi_listener = {
    .configure = ivi_window::configure,
};

void ivi_window::enter_output(void *, wl_surface *, wl_output *) {}
void ivi_window::leave_output(void *data, wl_surface *, wl_output *output) {
  auto *self = reinterpret_cast<ivi_window *>(data);
  self->display(*output);
}

void ivi_window::configure(void *data, ivi_surface *, int32_t width,
                           int32_t height) {
  auto *self = reinterpret_cast<ivi_window *>(data);
  self->resize({width, height});
}
