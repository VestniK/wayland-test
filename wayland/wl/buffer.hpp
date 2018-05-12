#pragma once

#include "basic_resource.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename Buffer>
struct buffer {
  version get_version() const noexcept {
    return version{wl_buffer_get_version(native_handle<Buffer>(*this))};
  }
};

}

using buffer = detail::basic_resource<wl_buffer, detail::buffer>;

}
