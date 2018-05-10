#pragma once

#include <wayland-client.h>

#include "basic_listener.hpp"
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

  void pong(uint32_t serial) {wl_shell_surface_pong(native_handle<ShellSurface>(*this), serial);}

  template<typename F>
  class listener: public basic_listener<wl_shell_surface_listener, F> {
  public:
    template<typename... A>
    listener(A&&... a):
      basic_listener<wl_shell_surface_listener, F>{
        wl_shell_surface_listener{&ping, &configure, &popup_done}, std::forward<A>(a)...
      }
    {}

  private:
    static void ping(void *data, wl_shell_surface *shell_surface, uint32_t serial) {
      auto* self = static_cast<listener*>(data);
      self->get_function().ping(resource_ref_t<ShellSurface>{*shell_surface}, serial);
    }

    static void configure(void *data, wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
      auto* self = static_cast<listener*>(data);
      self->get_function().configure(resource_ref_t<ShellSurface>{*shell_surface}, edges, width, height);
    }

    static void popup_done(void *data, wl_shell_surface *shell_surface) {
      auto* self = static_cast<listener*>(data);
      self->get_function().popup_done(resource_ref_t<ShellSurface>{*shell_surface});
    }
  };
  template<typename F> listener(F&&) -> listener<std::decay_t<F>>;

  template<typename F>
  void add_listener(listener<F>& listener) {
    if (::wl_shell_surface_add_listener(native_handle<ShellSurface>(*this), &listener.native_listener(), &listener) != 0)
      throw std::system_error{errc::add_shm_listener_failed};
  }
};

}

using shell_surface = detail::basic_resource<wl_shell_surface, detail::shell_surface>;

}
