#pragma once

#include "basic_resource.hpp"
#include "util.hpp"

namespace wl::detail {

template<typename Pool>
struct shm_pool {
  version get_version() const noexcept {
    return version{wl_shm_pool_get_version(native_handle<Pool>(*this))};
  }

  unique_ptr<wl_buffer> create_buffer(int32_t offset, int32_t width, int32_t height, int32_t stride, format fmt) {
    return unique_ptr<wl_buffer>{
      wl_shm_pool_create_buffer(native_handle<Pool>(*this), offset, width, height, stride, static_cast<uint32_t>(fmt))
    };
  }
};

}
