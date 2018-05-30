#pragma once

#include <functional>

#include <wayland-client.h>

#include "error.hpp"
#include "util.hpp"
#include "basic_resource.hpp"

namespace wl {
namespace detail {

template<typename Callback>
struct callback {
  template<typename F>
  void add_listener(F& l) {
    static const wl_callback_listener listener {
      [](void* data, wl_callback* cb, uint32_t cb_data) {
        auto self = static_cast<F*>(data);
        std::invoke(*self, resource_ref_t<Callback>{*cb}, cb_data);
      }
    };
    if (wl_callback_add_listener(native_handle<Callback>(*this), &listener, &l) < 0)
      throw std::system_error{errc::add_callback_listener_failed};
  }
};

}

using callback = detail::basic_resource<wl_callback, detail::callback>;

}
