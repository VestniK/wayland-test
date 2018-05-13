#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#include <wayland-client.h>

namespace wl {

struct clock {
  using duration = std::chrono::milliseconds;
  using time_point = std::chrono::time_point<clock, duration>;
  using rep = duration::rep;
  using period = duration::period;
  static constexpr bool is_steady = true;
};

template<typename E>
auto underlying_cast(E e) noexcept {return static_cast<std::underlying_type_t<E>>(e);}

enum class id: uint32_t {};
enum class version: uint32_t {};
enum class serial: uint32_t {};
inline std::ostream& operator<< (std::ostream& out, version ver) {return out << underlying_cast(ver);}
inline std::ostream& operator<< (std::ostream& out, serial val) {
  const auto flags = out.flags();
  out << std::hex << underlying_cast(val);
  out.setf(flags);
  return out;
}

struct deleter {
  void operator() (wl_buffer* ptr) {wl_buffer_destroy(ptr);}
  void operator() (wl_callback* ptr) {wl_callback_destroy(ptr);}
  void operator() (wl_compositor* ptr) {wl_compositor_destroy(ptr);}
  void operator() (wl_display* ptr) {wl_display_disconnect(ptr);}
  void operator() (wl_keyboard* ptr) {wl_keyboard_destroy(ptr);}
  void operator() (wl_registry* ptr) {wl_registry_destroy(ptr);}
  void operator() (wl_seat* ptr) {wl_seat_destroy(ptr);}
  void operator() (wl_shell* ptr) {wl_shell_destroy(ptr);}
  void operator() (wl_shell_surface* ptr) {wl_shell_surface_destroy(ptr);}
  void operator() (wl_shm* ptr) {wl_shm_destroy(ptr);}
  void operator() (wl_shm_pool* ptr) {wl_shm_pool_destroy(ptr);}
  void operator() (wl_surface* ptr) {wl_surface_destroy(ptr);}
};
template<typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

using std::literals::string_view_literals::operator""sv;

enum class format: uint32_t {
  // required to be supported by any valid compositor
  ARGB8888 = WL_SHM_FORMAT_ARGB8888,
  XRGB8888 = WL_SHM_FORMAT_XRGB8888,

  // optinal known formats
  C8 = WL_SHM_FORMAT_C8,
  RGB332 = WL_SHM_FORMAT_RGB332,
  BGR233 = WL_SHM_FORMAT_BGR233,
  XRGB4444 = WL_SHM_FORMAT_XRGB4444,
  XBGR4444 = WL_SHM_FORMAT_XBGR4444,
  RGBX4444 = WL_SHM_FORMAT_RGBX4444,
  BGRX4444 = WL_SHM_FORMAT_BGRX4444,
  ARGB4444 = WL_SHM_FORMAT_ARGB4444,
  ABGR4444 = WL_SHM_FORMAT_ABGR4444,
  RGBA4444 = WL_SHM_FORMAT_RGBA4444,
  BGRA4444 = WL_SHM_FORMAT_BGRA4444,
  XRGB1555 = WL_SHM_FORMAT_XRGB1555,
  XBGR1555 = WL_SHM_FORMAT_XBGR1555,
  RGBX5551 = WL_SHM_FORMAT_RGBX5551,
  BGRX5551 = WL_SHM_FORMAT_BGRX5551,
  ARGB1555 = WL_SHM_FORMAT_ARGB1555,
  ABGR1555 = WL_SHM_FORMAT_ABGR1555,
  RGBA5551 = WL_SHM_FORMAT_RGBA5551,
  BGRA5551 = WL_SHM_FORMAT_BGRA5551,
  RGB565 = WL_SHM_FORMAT_RGB565,
  BGR565 = WL_SHM_FORMAT_BGR565,
  RGB888 = WL_SHM_FORMAT_RGB888,
  BGR888 = WL_SHM_FORMAT_BGR888,
  XBGR8888 = WL_SHM_FORMAT_XBGR8888,
  RGBX8888 = WL_SHM_FORMAT_RGBX8888,
  BGRX8888 = WL_SHM_FORMAT_BGRX8888,
  ABGR8888 = WL_SHM_FORMAT_ABGR8888,
  RGBA8888 = WL_SHM_FORMAT_RGBA8888,
  BGRA8888 = WL_SHM_FORMAT_BGRA8888,
  XRGB2101010 = WL_SHM_FORMAT_XRGB2101010,
  XBGR2101010 = WL_SHM_FORMAT_XBGR2101010,
  RGBX1010102 = WL_SHM_FORMAT_RGBX1010102,
  BGRX1010102 = WL_SHM_FORMAT_BGRX1010102,
  ARGB2101010 = WL_SHM_FORMAT_ARGB2101010,
  ABGR2101010 = WL_SHM_FORMAT_ABGR2101010,
  RGBA1010102 = WL_SHM_FORMAT_RGBA1010102,
  BGRA1010102 = WL_SHM_FORMAT_BGRA1010102,
  YUYV = WL_SHM_FORMAT_YUYV,
  YVYU = WL_SHM_FORMAT_YVYU,
  UYVY = WL_SHM_FORMAT_UYVY,
  VYUY = WL_SHM_FORMAT_VYUY,
  AYUV = WL_SHM_FORMAT_AYUV,
  NV12 = WL_SHM_FORMAT_NV12,
  NV21 = WL_SHM_FORMAT_NV21,
  NV16 = WL_SHM_FORMAT_NV16,
  NV61 = WL_SHM_FORMAT_NV61,
  YUV410 = WL_SHM_FORMAT_YUV410,
  YVU410 = WL_SHM_FORMAT_YVU410,
  YUV411 = WL_SHM_FORMAT_YUV411,
  YVU411 = WL_SHM_FORMAT_YVU411,
  YUV420 = WL_SHM_FORMAT_YUV420,
  YVU420 = WL_SHM_FORMAT_YVU420,
  YUV422 = WL_SHM_FORMAT_YUV422,
  YVU422 = WL_SHM_FORMAT_YVU422,
  YUV444 = WL_SHM_FORMAT_YUV444,
  YVU444 = WL_SHM_FORMAT_YVU444
};

struct size {
  int32_t width = 0;
  int32_t height = 0;
};

}
