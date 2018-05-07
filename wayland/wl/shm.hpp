#pragma once

#include <string_view>

#include "basic_resource.hpp"
#include "error.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename SHM>
struct shm {
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_shm"sv;

  version get_version() const noexcept {
    return version{wl_shm_get_version(native_handle<SHM>(*this))};
  }

  template<typename F>
  class listener: public F {
  public:
    listener(): F() {}
    template<typename... A>
    listener(A&&... a): F(std::forward<F>(a)...) {}

    listener(const listener&) = delete;
    listener& operator= (const listener&) = delete;
    listener(listener&&) = delete;
    listener& operator= (listener&&) = delete;

    operator const wl_shm_listener& () const noexcept {return listener_;}

  private:
    F& get_function() {return static_cast<F&>(*this);}

    static void format(void* data, wl_shm* handle, uint32_t fmt) {
      listener* self = static_cast<listener*>(data);
      std::invoke(self->get_function(), resource_ref_t<SHM>{*handle}, fmt);
    }


    wl_shm_listener listener_ = {&format};
  };
  template<typename F> listener(F&&) -> listener<std::decay_t<F>>;

  template<typename F>
  void add_listener(listener<F>& listener) {
    const wl_shm_listener& native_listener = listener;
    if (::wl_shm_add_listener(native_handle<SHM>(*this), &native_listener, &listener) != 0)
      throw std::system_error{errc::add_shm_listener_failed};
  }
};
template<typename SHM>
const wl_interface& shm<SHM>::resource_interface = wl_shm_interface;

}

using shm = detail::basic_resource<wl_shm, detail::shm>;

};
