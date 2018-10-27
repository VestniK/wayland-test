#include <utility>

#include "window.h"
#include "wlutil.hpp"
#include "xdg_shell.h"

toplevel_window::~toplevel_window() = default;

toplevel_window::toplevel_window(wl_compositor& compositor, xdg_wm_base& shell):
  surface_{wl::unique_ptr<wl_surface>{wl_compositor_create_surface(&compositor)}},
  xdg_surface_{wl::unique_ptr<xdg_surface>{xdg_wm_base_get_xdg_surface(&shell, surface_.get())}},
  toplevel_{wl::unique_ptr<xdg_toplevel>{xdg_surface_get_toplevel(xdg_surface_.get())}}
{
  xdg_toplevel_add_listener(toplevel_.get(), &toplevel_listener, this);
  xdg_surface_add_listener(xdg_surface_.get(), &surface_listener, this);
}

xdg_toplevel_listener toplevel_window::toplevel_listener = {
  .configure = toplevel_window::configure_toplevel,
  .close = toplevel_window::close
};

xdg_surface_listener toplevel_window::surface_listener = {
  .configure = toplevel_window::configure_surface
};
