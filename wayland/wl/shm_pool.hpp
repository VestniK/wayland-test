#pragma once

#include "basic_resource.hpp"
#include "util.hpp"

namespace wl::detail {

template<typename Pool>
struct shm_pool {
  version get_version() const noexcept {
    return version{wl_shm_pool_get_version(native_handle<Pool>(*this))};
  }
};

}
