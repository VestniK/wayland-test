#pragma once

#include <wayland-client.h>

#include "basic_resource.hpp"

namespace wl {
namespace detail {

template<typename Registry>
struct registry {
  version get_version() const {
    return version{wl_registry_get_version(native_handle<Registry>(*this))};
  }

  template<typename Iface>
  Iface bind(id name, version ver) {
    return Iface{
      unique_ptr<typename Iface::resource_type>{static_cast<typename Iface::resource_type*>(
        ::wl_registry_bind(
          native_handle<Registry>(*this),
          underlying_cast(name),
          &Iface::resource_interface,
          underlying_cast(ver)
        )
      )}
    };
  }

  template<typename F>
  class listener;
  template<typename F> listener(F&&) -> listener<std::decay_t<F>>;

  template<typename F>
  void add_listener(listener<F>& l);
};

template<typename Registry>
template<typename F>
class registry<Registry>::listener: public F {
public:
  listener(): F() {}
  template<typename... A>
  listener(A&&... a): F(std::forward<F>(a)...) {}

  listener(const listener&) = delete;
  listener& operator= (const listener&) = delete;
  listener(listener&&) = delete;
  listener& operator= (listener&&) = delete;

  operator const wl_registry_listener& () const noexcept {return listener_;}

private:
  F& get_function() {return static_cast<F&>(*this);}

  static void on_added(void* data, wl_registry* handle, uint32_t id, const char* iface, uint32_t ver) {
    listener* self = static_cast<listener*>(data);
    std::invoke(
      self->get_function(), resource_ref_t<Registry>{*handle}, ::wl::id{id}, std::string_view{iface}, version{ver}
    );
  }

  static void on_removed(void* data, struct wl_registry* handle, uint32_t id) {
    listener* self = static_cast<listener*>(data);
    std::invoke(self->get_function(), resource_ref_t<Registry>{*handle}, ::wl::id{id});
  }

  wl_registry_listener listener_ = {&on_added, &on_removed};
};

template<typename Registry>
template<typename F>
void registry<Registry>::add_listener(listener<F>& l) {
  const wl_registry_listener& native_listener = l;
  if (::wl_registry_add_listener(native_handle<Registry>(*this), &native_listener, &l) != 0)
    throw std::system_error{errc::add_registry_listener_failed};
}

}

using registry = detail::basic_resource<wl_registry, detail::registry>;

}
