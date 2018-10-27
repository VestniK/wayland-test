#pragma once

#include <memory>
#include <string_view>
#include <type_traits>

#include <wayland-client.h>
#include <wayland-egl.h>

#include "geom.h"
#include "xdg_shell.h"

namespace wl {

using std::literals::operator "" sv;

struct deleter {
  void operator() (wl_buffer* ptr) noexcept {wl_buffer_destroy(ptr);}
  void operator() (wl_callback* ptr) noexcept {wl_callback_destroy(ptr);}
  void operator() (wl_compositor* ptr) noexcept {wl_compositor_destroy(ptr);}
  void operator() (wl_display* ptr) noexcept {wl_display_disconnect(ptr);}
  void operator() (wl_egl_window* ptr) noexcept {wl_egl_window_destroy(ptr);}
  void operator() (wl_event_queue* ptr) noexcept {wl_event_queue_destroy(ptr);}
  void operator() (wl_keyboard* ptr) noexcept {wl_keyboard_destroy(ptr);}
  void operator() (wl_output* ptr) noexcept {wl_output_destroy(ptr);}
  void operator() (wl_pointer* ptr) noexcept {wl_pointer_destroy(ptr);}
  void operator() (wl_registry* ptr) noexcept {wl_registry_destroy(ptr);}
  void operator() (wl_seat* ptr) noexcept {wl_seat_destroy(ptr);}
  void operator() (wl_shell* ptr) noexcept {wl_shell_destroy(ptr);}
  void operator() (wl_shell_surface* ptr) noexcept {wl_shell_surface_destroy(ptr);}
  void operator() (wl_shm* ptr) noexcept {wl_shm_destroy(ptr);}
  void operator() (wl_shm_pool* ptr) noexcept {wl_shm_pool_destroy(ptr);}
  void operator() (wl_subcompositor* ptr) noexcept {wl_subcompositor_destroy(ptr);}
  void operator() (wl_subsurface* ptr) noexcept {wl_subsurface_destroy(ptr);}
  void operator() (wl_surface* ptr) noexcept {wl_surface_destroy(ptr);}

  void operator() (xdg_wm_base* ptr) noexcept {xdg_wm_base_destroy(ptr);}
  void operator() (xdg_surface* ptr) noexcept {xdg_surface_destroy(ptr);}
  void operator() (xdg_toplevel* ptr) noexcept {xdg_toplevel_destroy(ptr);}
};
template<typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

template<typename Service>
struct service_trait;

template<>
struct service_trait<wl_compositor> {
  static constexpr auto name = "wl_compositor"sv;
  static constexpr const wl_interface* iface = &wl_compositor_interface;
};

template<>
struct service_trait<wl_shell> {
  static constexpr auto name = "wl_shell"sv;
  static constexpr const wl_interface* iface = &wl_shell_interface;
};

template<>
struct service_trait<wl_output> {
  static constexpr auto name = "wl_output"sv;
  static constexpr const wl_interface* iface = &wl_output_interface;
};

template<>
struct service_trait<xdg_wm_base> {
  static constexpr auto name = "xdg_wm_base"sv;
  static constexpr const wl_interface* iface = &xdg_wm_base_interface;
};

template<typename Service>
auto bind(wl_registry* reg, uint32_t id, uint32_t ver) {
  return unique_ptr<Service>{reinterpret_cast<Service*>(wl_registry_bind(reg, id, service_trait<Service>::iface, ver))};
}

enum class fixed_t: wl_fixed_t {};
inline int to_int(fixed_t val) noexcept {return wl_fixed_to_int(static_cast<wl_fixed_t>(val));}
inline double to_double(fixed_t val) noexcept {return wl_fixed_to_double(static_cast<wl_fixed_t>(val));}
inline fixed_t to_fixed(int val) noexcept {return fixed_t{wl_fixed_from_int(val)};}
inline fixed_t to_fixed(double val) noexcept {return fixed_t{wl_fixed_from_double(val)};}

using fixed_point = basic_point<fixed_t>;

template<typename T>
constexpr bool operator== (basic_size<T> lhs, basic_size<T> rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height;
}

enum class millimeters: int32_t {};
using physical_size = basic_size<millimeters>;

}
