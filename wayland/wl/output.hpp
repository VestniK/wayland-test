#pragma once

#include <util/bitmask.hpp>

#include "basic_resource.hpp"

namespace wl {

enum class subpixel {
  unknown = WL_OUTPUT_SUBPIXEL_UNKNOWN,
  none = WL_OUTPUT_SUBPIXEL_NONE,
  horizontal_rgb = WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB,
  horizontal_bgr = WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR,
  vertical_rgb = WL_OUTPUT_SUBPIXEL_VERTICAL_RGB,
  vertical_bgr = WL_OUTPUT_SUBPIXEL_VERTICAL_BGR
};

enum class transform {
  normal = WL_OUTPUT_TRANSFORM_NORMAL,
  rotated_90 = WL_OUTPUT_TRANSFORM_90,
  rotated_180 = WL_OUTPUT_TRANSFORM_180,
  rotated_270 = WL_OUTPUT_TRANSFORM_270,
  flipped = WL_OUTPUT_TRANSFORM_FLIPPED,
  flipped_90 = WL_OUTPUT_TRANSFORM_FLIPPED_90,
  flipped_180 = WL_OUTPUT_TRANSFORM_FLIPPED_180,
  flipped_270 = WL_OUTPUT_TRANSFORM_FLIPPED_270
};

namespace detail {

template<typename Output>
struct output {
  using resource_type = wl_output;
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_output"sv;

  using subpixel = wl::subpixel;
  using transform = wl::transform;

  enum class mode: uint32_t {
    current = WL_OUTPUT_MODE_CURRENT,
    preferred = WL_OUTPUT_MODE_PREFERRED
  };
  using mode_flags = ut::bitmask<mode>;

  template<typename F>
  void add_listener(F& listener) {
    static const wl_output_listener static_listener = {
      [](
        void* data, wl_output* output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
        int32_t subpix, const char* make, const char *model, int32_t trans
      ) {
        auto* self = static_cast<F*>(data);
        self->geometry(
          resource_ref_t<Output>{*output}, point{x, y},
          physical_size{millimeters{physical_width}, millimeters{physical_height}},
          subpixel{subpix}, make, model, transform{trans}
        );
      },
      [](void* data, wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
        auto* self = static_cast<F*>(data);
        self->mode(resource_ref_t<Output>{*output}, mode_flags{flags}, size{width, height}, refresh);
      },
      [](void *data, wl_output* output) {
        auto* self = static_cast<F*>(data);
        self->done(resource_ref_t<Output>{*output});
      },
      [](void *data, wl_output* output, int32_t factor) {
        auto* self = static_cast<F*>(data);
        self->scale(resource_ref_t<Output>{*output}, factor);
      }
    };
    if (wl_output_add_listener(native_handle<Output>(*this), &static_listener, &listener) != 0)
      throw std::system_error{errc::add_output_listener_failed};
  }

};
template<typename Output>
const wl_interface& output<Output>::resource_interface = wl_output_interface;

}

using output = detail::basic_resource<wl_output, detail::output>;

}
