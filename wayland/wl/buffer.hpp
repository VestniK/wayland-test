#pragma once

#include "basic_resource.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename Buffer>
struct buffer {

};

}

using buffer = detail::basic_resource<wl_buffer, detail::buffer>;

}
