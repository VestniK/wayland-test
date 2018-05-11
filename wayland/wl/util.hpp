#pragma once

#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#include <wayland-client.h>

namespace wl {

template<typename E>
auto underlying_cast(E e) noexcept {return static_cast<std::underlying_type_t<E>>(e);}

enum class id: uint32_t {};
enum class version: uint32_t {};
inline std::ostream& operator<< (std::ostream& out, version ver) {return out << underlying_cast(ver);}

struct deleter {
  void operator() (wl_display* ptr) {wl_display_disconnect(ptr);}
  void operator() (wl_registry* ptr) {wl_registry_destroy(ptr);}
  void operator() (wl_compositor* ptr) {wl_compositor_destroy(ptr);}
  void operator() (wl_shell* ptr) {wl_shell_destroy(ptr);}
  void operator() (wl_callback* ptr) {wl_callback_destroy(ptr);}
  void operator() (wl_surface* ptr) {wl_surface_destroy(ptr);}
  void operator() (wl_shell_surface* ptr) {wl_shell_surface_destroy(ptr);}
  void operator() (wl_shm* ptr) {wl_shm_destroy(ptr);}
  void operator() (wl_shm_pool* ptr) {wl_shm_pool_destroy(ptr);}
};
template<typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

using std::literals::string_view_literals::operator""sv;

}
