#pragma once

#include <chrono>

#include "basic_resource.hpp"
#include "basic_listener.hpp"
#include "error.hpp"
#include "surface.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

enum class keymap_format: uint32_t {
  no_keymap = WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP,
  xcb_v1 = WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1
};

enum class key_state: uint32_t {
  released = WL_KEYBOARD_KEY_STATE_RELEASED,
  pressed = WL_KEYBOARD_KEY_STATE_PRESSED
};

template<typename KB>
struct keyboard {
  using keymap_format = keymap_format;
  using key_state = key_state;

  version get_version() const noexcept {
    return version{wl_keyboard_get_version(native_handle<KB>(*this))};
  }

  template<typename F>
  class listener: basic_listener<wl_keyboard_listener, F> {
  public:
    template<typename... A>
    listener(A&&... a):
      basic_listener<wl_keyboard_listener, F>{
        {&keymap, &enter, &leave, &key, &modifiers, &repeat_info}, std::forward<F>(a)...
      }
    {}
  private:
    static void keymap(void* data, wl_keyboard* handle, uint32_t fmt, int32_t fd, uint32_t size) {
      listener* self = static_cast<listener*>(data);
      self->get_function().keymap(resource_ref_t<KB>{*handle}, keymap_format{fmt}, fd, size);
    }
    static void enter(void* data, wl_keyboard* handle, uint32_t eid, wl_surface* surf, wl_array* pressed_keys) {
      listener* self = static_cast<listener*>(data);
      self->get_function().enter(resource_ref_t<KB>{*handle}, serial{eid}, wl::surface::ref(*surf), pressed_keys);
    }
    static void leave(void* data, wl_keyboard* handle, uint32_t eid, wl::surface::ref(*surf)) {
      listener* self = static_cast<listener*>(data);
      self->get_function().leave(resource_ref_t<KB>{*handle}, serial{eid}, wl::surface::ref(*surf));
    }
    static void key(void* data, wl_keyboard* handle, uint32_t eid, uint32_t time, uint32_t key, uint32_t state) {
      listener* self = static_cast<listener*>(data);
      self->get_function().key(
        resource_ref_t<KB>{*handle}, serial{eid}, clock::time_point{clock::duration{time}}, key, key_state{state});
    }
    static void modifiers(
      void* data, wl_keyboard* handle, uint32_t eid, uint32_t mods_depressed, uint32_t mods_latched,
      uint32_t mods_locked, uint32_t group
    ) {
      listener* self = static_cast<listener*>(data);
      self->get_function().modifiers(
        resource_ref_t<KB>{*handle}, serial{eid}, mods_depressed, mods_latched, mods_locked, group
      );
    }
    static void repeat_info(void* data, wl_keyboard* handle, int32_t rate, int32_t delay) {
      listener* self = static_cast<listener*>(data);
      self->get_function().key(resource_ref_t<KB>{*handle}, rate, std::chrono::milliseconds(delay));
    }
  };
  template<typename F> listener(F&&) -> listener<std::decay_t<F>>;

  template<typename F>
  void add_listener(listener<F>& listener) {
    if (wl_keyboard_add_listener(native_handle<KB>(*this), &listener.native_listener(), &listener) != 0)
      throw std::system_error{errc::add_keyboard_listener_failed};
  }
};

}

using keyboard = detail::basic_resource<wl_keyboard, detail::keyboard>;

}
