#pragma once

#include "basic_resource.hpp"
#include "util.hpp"

#include "buffer.hpp"

namespace wl::detail {

template<typename Pool>
struct shm_pool {
  version get_version() const noexcept {
    return version{wl_shm_pool_get_version(native_handle<Pool>(*this))};
  }

  wl::buffer create_buffer(int32_t offset, size sz, int32_t stride, format fmt) {
    return unique_ptr<wl_buffer>{wl_shm_pool_create_buffer(
      native_handle<Pool>(*this), offset, sz.width, sz.height, stride, static_cast<uint32_t>(fmt)
    )};
  }
};

}
