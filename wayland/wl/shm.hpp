#pragma once

#include <string_view>

#include "basic_resource.hpp"
#include "error.hpp"
#include "shm_pool.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename SHM>
struct shm {
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_shm"sv;

  using format = wl::format;

  version get_version() const noexcept {
    return version{wl_shm_get_version(native_handle<SHM>(*this))};
  }

  using pool = basic_resource<wl_shm_pool, shm_pool>;
  pool create_pool(int32_t fd, int32_t size) {
    return unique_ptr<wl_shm_pool>{wl_shm_create_pool(native_handle<SHM>(*this), fd, size)};
  }

  template<typename F>
  void add_listener(F& listener) {
    static const wl_shm_listener static_listener = {
      [](void* data, wl_shm* handle, uint32_t fmt) {
        auto self = static_cast<F*>(data);
        std::invoke(*self, resource_ref_t<SHM>{*handle}, wl::format{fmt});
      }
    };
    if (wl_shm_add_listener(native_handle<SHM>(*this), &static_listener, &listener) != 0)
      throw std::system_error{errc::add_shm_listener_failed};
  }
};
template<typename SHM>
const wl_interface& shm<SHM>::resource_interface = wl_shm_interface;

}

using shm = detail::basic_resource<wl_shm, detail::shm>;

};
