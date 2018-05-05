#pragma once

#include <wayland-client.h>

#include "error.hpp"
#include "util.hpp"

namespace wl {

template<typename Registry>
struct registry_interface {
  version get_version() const {
    return version{wl_registry_get_version(static_cast<const Registry&>(*this).native_handle())};
  }

  template<typename Iface>
  Iface bind(id name, version ver) {
    return Iface{
      unique_ptr<typename Iface::resource_type>{static_cast<typename Iface::resource_type*>(
        ::wl_registry_bind(
          static_cast<Registry&>(*this).native_handle(),
          underlying_cast(name),
          &Iface::resource_interface,
          underlying_cast(ver)
        )
      )}
    };
  }
};

class registry_ref: public registry_interface<registry_ref> {
public:
  using native_handle_type = wl_registry*;
  native_handle_type native_handle() const {return &ref_.get();}

  registry_ref(native_handle_type handle): ref_(*handle) {}

private:
  std::reference_wrapper<wl_registry> ref_;
};

class registry: public registry_interface<registry> {
public:
  using native_handle_type = wl_registry*;

  explicit registry(unique_ptr<wl_registry> inst): ptr_(std::move(inst)) {}

  wl_registry* native_handle() const {return ptr_.get();}

  template<typename F>
  class listener;
  template<typename F> listener(F&&) -> listener<std::decay_t<F>>;

  template<typename F>
  void add_listener(listener<F>& l);

private:
  unique_ptr<wl_registry> ptr_;
};

template<typename F>
class registry::listener: public F {
public:
  listener(): F() {}
  template<typename... A>
  listener(A&&... a): F(std::forward<F>(a)...) {}

  listener(const listener&) = delete;
  listener& operator= (const listener&) = delete;
  listener(listener&&) = delete;
  listener& operator= (listener&&) = delete;

private:
  F& get_function() {return static_cast<F&>(*this);}

  static void on_added(void* data, wl_registry* handle, uint32_t id, const char* iface, uint32_t ver) {
    listener* self = static_cast<listener*>(data);
    std::invoke(self->get_function(), registry_ref{handle}, ::wl::id{id}, std::string_view{iface}, version{ver});
  }

  static void on_removed(void* data, struct wl_registry* handle, uint32_t id) {
    listener* self = static_cast<listener*>(data);
    std::invoke(self->get_function(), registry_ref{handle}, ::wl::id{id});
  }

  friend class ::wl::registry;

  wl_registry_listener listener_ = {&on_added, &on_removed};
};

template<typename F>
void registry::add_listener(listener<F>& l) {
  if (::wl_registry_add_listener(ptr_.get(), &l.listener_, &l) != 0)
    throw std::system_error{errc::add_registry_listener_failed};
}

}
