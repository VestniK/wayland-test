#include <utility>

#include <ivi-application.h>

#include <libs/wlwnd/ivi_window.hpp>
#include <libs/wlwnd/wlutil.hpp>

namespace ivi {

window::window(wl_compositor& compositor, ivi_application& shell,
    uint32_t ivi_id, delegate* delegate)
    : surface_{wl::unique_ptr<wl_surface>{
          wl_compositor_create_surface(&compositor)}},
      ivi_surface_{wl::unique_ptr<ivi_surface>{
          ivi_application_surface_create(&shell, ivi_id, surface_.get())}} {
  ivi_surface_add_listener(ivi_surface_.get(), &ivi_listener, delegate);
}

void window::set_delegate(delegate* delegate) noexcept {
  wl_proxy_set_user_data(
      reinterpret_cast<wl_proxy*>(ivi_surface_.get()), delegate);
}

void window::set_delegate_queue(wl_event_queue& queue) noexcept {
  wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(ivi_surface_.get()), &queue);
}

ivi_surface_listener window::ivi_listener = {
    .configure = window::configure,
};

void window::configure(
    void* data, ivi_surface*, int32_t width, int32_t height) {
  if (auto* delegate = reinterpret_cast<ivi::delegate*>(data))
    delegate->resize({width, height});
}

} // namespace ivi
