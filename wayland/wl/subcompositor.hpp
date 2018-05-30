#pragma once

#include <string_view>

#include "basic_resource.hpp"
#include "subsurface.hpp"
#include "surface.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

template<typename SubCmpz>
struct subcompositor {
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_subcompositor"sv;

  wl::subsurface get_subsurface(wl::surface::ref surf, wl::surface::ref parent) {
    return unique_ptr<wl_subsurface>{
      wl_subcompositor_get_subsurface(native_handle<SubCmpz>(*this), surf.native_handle(), parent.native_handle())
    };
  }
};
template<typename SubCmpz>
const wl_interface& subcompositor<SubCmpz>::resource_interface = wl_subcompositor_interface;

}

using subcompositor = detail::basic_resource<wl_subcompositor, detail::subcompositor>;

}
