#include <xdg-shell.h>

#include <libs/wlwnd/xdg_window.hpp>

namespace xdg {

toplevel_window::toplevel_window(wl_compositor& compositor, xdg_wm_base& shell, delegate* delegate)
    : surface_{wl::unique_ptr<wl_surface>{wl_compositor_create_surface(&compositor)}},
      xdg_surface_{wl::unique_ptr<xdg_surface>{xdg_wm_base_get_xdg_surface(&shell, surface_.get())}},
      toplevel_{wl::unique_ptr<xdg_toplevel>{xdg_surface_get_toplevel(xdg_surface_.get())}} {
  xdg_toplevel_add_listener(toplevel_.get(), &toplevel_listener, delegate);
  xdg_surface_add_listener(xdg_surface_.get(), &surface_listener, delegate);
}

void toplevel_window::set_delegate(delegate* delegate) noexcept {
  wl_proxy_set_user_data(reinterpret_cast<wl_proxy*>(xdg_surface_.get()), delegate);
  wl_proxy_set_user_data(reinterpret_cast<wl_proxy*>(toplevel_.get()), delegate);
}

void toplevel_window::set_delegate_queue(wl_event_queue& queue) noexcept {
  wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(toplevel_.get()), &queue);
}

xdg_toplevel_listener toplevel_window::toplevel_listener = {
    .configure = toplevel_window::configure_toplevel,
    .close = toplevel_window::close,
    .configure_bounds = toplevel_window::configure_bounds,
    .wm_capabilities = toplevel_window::wm_capabilities
};

xdg_surface_listener toplevel_window::surface_listener = {.configure = toplevel_window::configure_surface};

wl_surface_listener toplevel_window::surface_output_listener = {
    .enter = toplevel_window::enter_output,
    .leave = toplevel_window::leave_output,
};

void toplevel_window::configure_surface(void*, xdg_surface* surf, uint32_t serial) {
  xdg_surface_ack_configure(surf, serial);
}

void toplevel_window::
    configure_toplevel(void* data, xdg_toplevel*, int32_t width, int32_t height, wl_array*) {
  auto* self = reinterpret_cast<delegate*>(data);
  self->resize({width, height});
}
void toplevel_window::close(void* data, xdg_toplevel*) {
  auto* self = reinterpret_cast<delegate*>(data);
  self->close();
}
void toplevel_window::configure_bounds(
    void* data [[maybe_unused]], xdg_toplevel* xdg_toplevel [[maybe_unused]], int32_t width [[maybe_unused]],
    int32_t height [[maybe_unused]]
) {}
void toplevel_window::wm_capabilities(
    void* data [[maybe_unused]], xdg_toplevel* xdg_toplevel [[maybe_unused]],
    wl_array* capabilities [[maybe_unused]]
) {}

void toplevel_window::enter_output(void*, wl_surface*, wl_output*) {}
void toplevel_window::leave_output(void* data, wl_surface*, wl_output* output) {
  auto* self = reinterpret_cast<delegate*>(data);
  self->display(*output);
}

} // namespace xdg
