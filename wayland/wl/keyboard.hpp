#pragma once

#include <chrono>

#include "basic_resource.hpp"
#include "error.hpp"
#include "surface.hpp"
#include "util.hpp"

namespace wl {

enum class keycode: uint32_t {};

namespace detail {

template<typename KB>
struct keyboard {
  using keymap_format = keymap_format;
  using key_state = key_state;

  version get_version() const noexcept {
    return version{wl_keyboard_get_version(native_handle<KB>(*this))};
  }

  template<typename F>
  void add_listener(F& listener) {
    static const wl_keyboard_listener static_listener= {
      [](void* data, wl_keyboard* handle, uint32_t fmt, int32_t fd, uint32_t size) {
        auto self = static_cast<F*>(data);
        self->keymap(resource_ref_t<KB>{*handle}, keymap_format{fmt}, fd, size);
      },
      [](void* data, wl_keyboard* handle, uint32_t eid, wl_surface* surf, wl_array* pressed_keys) {
        auto self = static_cast<F*>(data);
        self->enter(resource_ref_t<KB>{*handle}, serial{eid}, wl::surface::ref(*surf), pressed_keys);
      },
      [](void* data, wl_keyboard* handle, uint32_t eid, wl_surface* surf) {
        auto self = static_cast<F*>(data);
        self->leave(resource_ref_t<KB>{*handle}, serial{eid}, wl::surface::ref(*surf));
      },
      [](void* data, wl_keyboard* handle, uint32_t eid, uint32_t time, uint32_t key, uint32_t state) {
        auto self = static_cast<F*>(data);
        self->key(
          resource_ref_t<KB>{*handle}, serial{eid}, clock::time_point{clock::duration{time}},
          keycode{key}, key_state{state}
        );
      },
      [](
        void* data, wl_keyboard* handle, uint32_t eid, uint32_t mods_depressed, uint32_t mods_latched,
        uint32_t mods_locked, uint32_t group
      ) {
        auto self = static_cast<F*>(data);
        self->modifiers(
          resource_ref_t<KB>{*handle}, serial{eid}, mods_depressed, mods_latched, mods_locked, group
        );
      },
      [](void* data, wl_keyboard* handle, int32_t rate, int32_t delay) {
        auto self = static_cast<F*>(data);
        self->repeat_info(resource_ref_t<KB>{*handle}, rate, std::chrono::milliseconds(delay));
      }
    };
    if (wl_keyboard_add_listener(native_handle<KB>(*this), &static_listener, &listener) != 0)
      throw std::system_error{errc::add_keyboard_listener_failed};
  }
};

}

using keyboard = detail::basic_resource<wl_keyboard, detail::keyboard>;

}
