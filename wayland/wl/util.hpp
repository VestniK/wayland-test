#pragma once

#include <memory>
#include <type_traits>

#include <wayland-client.h>
#include <wayland-egl.h>

namespace wl {

struct deleter {
  void operator() (wl_buffer* ptr) {wl_buffer_destroy(ptr);}
  void operator() (wl_callback* ptr) {wl_callback_destroy(ptr);}
  void operator() (wl_compositor* ptr) {wl_compositor_destroy(ptr);}
  void operator() (wl_display* ptr) {wl_display_disconnect(ptr);}
  void operator() (wl_egl_window* ptr) noexcept {wl_egl_window_destroy(ptr);}
  void operator() (wl_event_queue* ptr) {wl_event_queue_destroy(ptr);}
  void operator() (wl_keyboard* ptr) {wl_keyboard_destroy(ptr);}
  void operator() (wl_output* ptr) {wl_output_destroy(ptr);}
  void operator() (wl_pointer* ptr) {wl_pointer_destroy(ptr);}
  void operator() (wl_registry* ptr) {wl_registry_destroy(ptr);}
  void operator() (wl_seat* ptr) {wl_seat_destroy(ptr);}
  void operator() (wl_shell* ptr) {wl_shell_destroy(ptr);}
  void operator() (wl_shell_surface* ptr) {wl_shell_surface_destroy(ptr);}
  void operator() (wl_shm* ptr) {wl_shm_destroy(ptr);}
  void operator() (wl_shm_pool* ptr) {wl_shm_pool_destroy(ptr);}
  void operator() (wl_subcompositor* ptr) {wl_subcompositor_destroy(ptr);}
  void operator() (wl_subsurface* ptr) {wl_subsurface_destroy(ptr);}
  void operator() (wl_surface* ptr) {wl_surface_destroy(ptr);}
};
template<typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

template<typename T>
struct basic_point {
  T x = 0;
  T y = 0;
};

enum class fixed_t: wl_fixed_t {};
inline int to_int(fixed_t val) noexcept {return wl_fixed_to_int(static_cast<wl_fixed_t>(val));}
inline double to_double(fixed_t val) noexcept {return wl_fixed_to_double(static_cast<wl_fixed_t>(val));}
inline fixed_t to_fixed(int val) noexcept {return fixed_t{wl_fixed_from_int(val)};}
inline fixed_t to_fixed(double val) noexcept {return fixed_t{wl_fixed_from_double(val)};}

using point = basic_point<int32_t>;
using fixed_point = basic_point<fixed_t>;

template<typename T>
struct basic_size {
  T width = {};
  T height = {};
};

using size = basic_size<int32_t>;

enum class millimeters: int32_t {};
using physical_size = basic_size<millimeters>;

struct rect {
  point top_left;
  ::wl::size size;
};

}
