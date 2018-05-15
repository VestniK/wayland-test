#pragma once

#include <wayland-client.h>

#include <util/util.hpp>

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
          ut::underlying_cast(name),
          &Iface::resource_interface,
          ut::underlying_cast(ver)
        )
      )}
    };
  }

  template<typename F>
  void add_listener(F& l) {
    static const wl_registry_listener listener = {
      [](void* data, wl_registry* handle, uint32_t id, const char* iface, uint32_t ver) {
        F* self = static_cast<F*>(data);
        self->global(resource_ref_t<Registry>{*handle}, ::wl::id{id}, std::string_view{iface}, version{ver});
      },
      [](void* data, wl_registry* handle, uint32_t id) {
        F* self = static_cast<F*>(data);
        self->global_remove(resource_ref_t<Registry>{*handle}, ::wl::id{id});
      }
    };

    wl_registry_add_listener(native_handle<Registry>(*this), &listener, &l);
  }
};

}

using registry = detail::basic_resource<wl_registry, detail::registry>;

}
