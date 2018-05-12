#pragma once

#include <string_view>

#include "basic_listener.hpp"
#include "basic_resource.hpp"
#include "bitmask.hpp"
#include "error.hpp"
#include "keyboard.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

enum class seat_capability: uint32_t {
  pointer = WL_SEAT_CAPABILITY_POINTER,
  keyboard = WL_SEAT_CAPABILITY_KEYBOARD,
  touch = WL_SEAT_CAPABILITY_TOUCH
};
constexpr bitmask<seat_capability> operator | (seat_capability l, seat_capability r) noexcept {
  return bitmask<seat_capability>(l) | r;
}

template<typename Seat>
struct seat {
  static const wl_interface& resource_interface;
  static constexpr std::string_view interface_name = "wl_seat"sv;

  using capability = seat_capability;

  version get_version() const noexcept {
    return version{wl_seat_get_version(native_handle<Seat>(*this))};
  }

  template<typename F>
  class listener: public basic_listener<wl_seat_listener, F> {
  public:
    template<typename... A>
    listener(A&&... a): basic_listener<wl_seat_listener, F>{{&capabilities, &name}, std::forward<F>(a)...} {}

  private:
    static void capabilities(void* data, wl_seat* handle, uint32_t caps) {
      listener* self = static_cast<listener*>(data);
      self->get_function().capabilities(resource_ref_t<Seat>{*handle}, bitmask<capability>{caps});
    }
    static void name(void* data, wl_seat* handle, const char* name) {
      listener* self = static_cast<listener*>(data);
      self->get_function().name(resource_ref_t<Seat>{*handle}, name);
    }
  };
  template<typename F> listener(F&&) -> listener<std::decay_t<F>>;

  template<typename F>
  void add_listener(listener<F>& listener) {
    if (wl_seat_add_listener(native_handle<Seat>(*this), &listener.native_listener(), &listener) != 0)
      throw std::system_error{errc::add_seat_listener_failed};
  }

  wl::keyboard get_keyboard() {
    return unique_ptr<wl_keyboard>{wl_seat_get_keyboard(native_handle<Seat>(*this))};
  }
};
template<typename Seat>
const wl_interface& seat<Seat>::resource_interface = wl_seat_interface;

}

using seat = detail::basic_resource<wl_seat, detail::seat>;

}
