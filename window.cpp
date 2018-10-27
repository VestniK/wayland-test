#include <utility>

#include "window.h"
#include "wlutil.hpp"
#include "xdg_shell.h"

window::delegate::~delegate() = default;

window::window(window&& rhs) noexcept:
  surface_{std::move(rhs.surface_)},
  toplevel_{std::move(rhs.toplevel_)}
{
  if (toplevel_)
    xdg_toplevel_set_user_data(toplevel_.get(), this);
}

window& window::operator= (window&& rhs) noexcept {
  surface_ = std::move(rhs.surface_);
  toplevel_ = std::move(rhs.toplevel_);
  if (toplevel_)
    xdg_toplevel_set_user_data(toplevel_.get(), this);
  return *this;
}

window window::create_toplevel(wl_compositor& compositor, xdg_wm_base& shell) {
  wl::unique_ptr<wl_surface> surface{wl_compositor_create_surface(&compositor)};
  wl::unique_ptr<xdg_surface> xdg_surf{xdg_wm_base_get_xdg_surface(&shell, surface.get())};
  wl::unique_ptr<xdg_toplevel> toplevel{xdg_surface_get_toplevel(xdg_surf.get())};
  return {std::move(surface), std::move(xdg_surf), std::move(toplevel)};
}

xdg_toplevel_listener window::toplevel_listener = {
  .configure = window::configure_toplevel,
  .close = window::close
};

xdg_surface_listener window::surface_listener = {
  .configure = window::configure_surface
};
