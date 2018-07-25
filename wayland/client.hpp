#pragma once

#include <string>
#include <string_view>
#include <system_error>

#include <wayland-client.h>

#include "wl/basic_resource.hpp"
#include "wl/buffer.hpp"
#include "wl/callback.hpp"
#include "wl/error.hpp"
#include "wl/event_queue.hpp"
#include "wl/io.hpp"
#include "wl/output.hpp"
#include "wl/registry.hpp"
#include "wl/seat.hpp"
#include "wl/shell_surface.hpp"
#include "wl/shm.hpp"
#include "wl/subcompositor.hpp"
#include "wl/subsurface.hpp"
#include "wl/surface.hpp"
#include "wl/util.hpp"

namespace wl {

namespace detail {

template<typename Compositor>
struct compositor {
  using resource_type = wl_compositor;
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_compositor"sv;

  basic_resource<wl_surface, surface> create_surface() {
    return unique_ptr<wl_surface>{wl_compositor_create_surface(native_handle<Compositor>(*this))};
  }
};
template<typename Compositor>
const wl_interface& compositor<Compositor>::resource_interface = wl_compositor_interface;

template<typename Shell>
struct shell {
  using resource_type = wl_shell;
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_shell"sv;

  explicit operator bool () const noexcept {return static_cast<bool>(ptr_);}

  basic_resource<wl_shell_surface, shell_surface> get_shell_surface(wl::surface::ref surf) {
    return {unique_ptr<wl_shell_surface>{wl_shell_get_shell_surface(native_handle<Shell>(*this), surf.native_handle())}};
  }

private:
  unique_ptr<resource_type> ptr_;
};
template<typename Shell>
const wl_interface& shell<Shell>::resource_interface = wl_shell_interface;

}

using compositor = detail::basic_resource<wl_compositor, detail::compositor>;
using shell = detail::basic_resource<wl_shell, detail::shell>;

} // namespace wl
