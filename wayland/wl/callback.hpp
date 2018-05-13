#pragma once

#include <wayland-client.h>

#include "error.hpp"
#include "util.hpp"
#include "basic_resource.hpp"

namespace wl {
namespace detail {

template<typename Callback>
struct callback {
  version get_version() const {
    return version{wl_callback_get_version(native_handle<Callback>(*this))};
  }

  template<typename F>
  class listener: private F {
  public:
    listener(): F() {}
    template<typename... A>
    listener(A&&... a): F(std::forward<A>(a)...) {}

    listener(const listener&) = delete;
    listener& operator= (const listener&) = delete;
    listener(listener&&) = delete;
    listener& operator= (listener&&) = delete;

    operator const wl_callback_listener& () const noexcept {return listener_;}

  private:
    F& get_function() noexcept {return static_cast<F&>(*this);}

    static void done(void* data, wl_callback* cb, uint32_t cb_data) {
      auto self = static_cast<listener *>(data);
      std::invoke(self->get_function(), resource_ref_t<Callback>{*cb}, cb_data);
    }

    wl_callback_listener listener_ = {&done};
  };
  template<typename F> listener(F&&) -> listener<std::decay_t<F>>;

  template<typename F>
  void add_listener(listener<F>& l) {
    const wl_callback_listener& native_listener = l;
    if (wl_callback_add_listener(native_handle<Callback>(*this), &native_listener, &l) < 0)
      throw std::system_error{errc::add_callback_listener_failed};
  }
};

}

using callback = detail::basic_resource<wl_callback, detail::callback>;

}
