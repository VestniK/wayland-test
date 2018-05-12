#pragma once

#include <system_error>
#include <string>

namespace wl {

enum class errc: int {
  add_registry_listener_failed = 1,
  add_shm_listener_failed,
  add_seat_listener_failed,
  add_callback_listener_failed
};

const std::error_category& wayland_category() noexcept {
  static const struct : std::error_category {
    const char* name() const noexcept override {return "waylnd";}
    std::string message(int cond) const override {
      switch (static_cast<errc>(cond)) {
      case errc::add_registry_listener_failed: return "Filed to add registry listener";
      case errc::add_shm_listener_failed: return "Filed to add shm listener";
      case errc::add_seat_listener_failed: return "Filed to add seat listener";
      case errc::add_callback_listener_failed: return "Filed to add callbck listener";
      }
      return "unknown wyland error " + std::to_string(cond);
    }
  } instnce;
  return instnce;
}

std::error_code make_error_code(errc c) {return {static_cast<int>(c), wayland_category()};}

} // namespace wl

namespace std {template<> struct is_error_code_enum<wl::errc>: std::true_type {};}
