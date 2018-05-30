#pragma once

#include <string_view>

#include "basic_resource.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename SubCmpz>
struct subcompositor {
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_subcompositor"sv;
};
template<typename SubCmpz>
const wl_interface& subcompositor<SubCmpz>::resource_interface = wl_subcompositor_interface;

}

using subcompositor = detail::basic_resource<wl_subcompositor, detail::subcompositor>;

}
