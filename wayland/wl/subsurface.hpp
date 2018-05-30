#pragma once

#include "basic_resource.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename Surf>
struct subsurface {
  void set_position(point pt) {
    wl_subsurface_set_position(native_handle<Surf>(*this), pt.x, pt.y);
  }

  void set_sync() {
    wl_subsurface_set_sync(native_handle<Surf>(*this));
  }

  void set_desync() {
    wl_subsurface_set_desync(native_handle<Surf>(*this));
  }
};

}

using subsurface = detail::basic_resource<wl_subsurface, detail::subsurface>;

}
