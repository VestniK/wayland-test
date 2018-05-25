#pragma once

#include <wayland-client.h>

#include "basic_resource.hpp"
#include "util.hpp"
#include "callback.hpp"

namespace wl {

namespace detail {

template<typename Surface>
struct surface {
  version get_version() const noexcept {
    return version{wl_surface_get_version(native_handle<Surface>(*this))};
  }

  void attach(wl::buffer::ref buf, point pt = {}) {
    wl_surface_attach(native_handle<Surface>(*this), buf.native_handle(), pt.x, pt.y);
  }

  void damage(rect area) {
    wl_surface_damage(
      native_handle<Surface>(*this), area.top_left.x, area.top_left.y, area.size.width, area.size.height
    );
  }

  wl::callback frame() {
    return unique_ptr<wl_callback>{wl_surface_frame(native_handle<Surface>(*this))};
  }

  void commit() {wl_surface_commit(native_handle<Surface>(*this));}
};

}

using surface = detail::basic_resource<wl_surface, detail::surface>;

}
