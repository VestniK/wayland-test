#pragma once

#include <wayland-client.h>

#include "basic_listener.hpp"
#include "basic_resource.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename Surface>
struct surface {
  version get_version() const noexcept {
    return version{wl_surface_get_version(native_handle<Surface>(*this))};
  }

  void attach(const wl::buffer& buf, int32_t x = 0, int32_t y = 0) {
    wl_surface_attach(native_handle<Surface>(*this), buf.native_handle(), x, y);
  }

  void commit() {wl_surface_commit(native_handle<Surface>(*this));}
};

}

using surface = detail::basic_resource<wl_surface, detail::surface>;

}
