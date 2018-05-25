#pragma once

#include <wayland-client.h>

#include <string>

#include <gsl/string_span>

#include <util/util.hpp>

#include "basic_resource.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename ShellSurface>
struct shell_surface {
  version get_version() const noexcept {
    return version{wl_shell_surface_get_version(native_handle<ShellSurface>(*this))};
  }

  void set_toplevel() const {
    wl_shell_surface_set_toplevel(native_handle<ShellSurface>(*this));
  }

  void pong(serial eid) {wl_shell_surface_pong(native_handle<ShellSurface>(*this), ut::underlying_cast(eid));}

  void set_title(const std::string& title) {
    wl_shell_surface_set_title(native_handle<ShellSurface>(*this), title.c_str());
  }

  void set_title(gsl::czstring<> title) {
    wl_shell_surface_set_title(native_handle<ShellSurface>(*this), title);
  }

  template<typename F>
  void add_listener(F& listener) {
    static const wl_shell_surface_listener static_listener = {
      [](void *data, wl_shell_surface *shell_surface, uint32_t eid) {
        auto* self = static_cast<F*>(data);
        self->ping(resource_ref_t<ShellSurface>{*shell_surface}, serial{eid});
      },
      [](void *data, wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
        auto* self = static_cast<F*>(data);
        self->configure(resource_ref_t<ShellSurface>{*shell_surface}, edges, size{width, height});
      },
      [](void *data, wl_shell_surface *shell_surface) {
        auto* self = static_cast<F*>(data);
        self->popup_done(resource_ref_t<ShellSurface>{*shell_surface});
      }
    };
    if (wl_shell_surface_add_listener(native_handle<ShellSurface>(*this), &static_listener, &listener) != 0)
      throw std::system_error{errc::add_shm_listener_failed};
  }
};

}

using shell_surface = detail::basic_resource<wl_shell_surface, detail::shell_surface>;

}
