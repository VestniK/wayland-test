#pragma once

#include <memory>

#include <wayland-egl.h>

#include "client.hpp"

namespace wl {
namespace egl {

struct deleter {
  void operator() (wl_egl_window* ptr) noexcept {wl_egl_window_destroy(ptr);}
};
template<typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

class window {
public:
  using native_handle_type = wl_egl_window*;

  window() noexcept = default;
  window(surface::ref surf, size sz): ptr_{wl_egl_window_create(surf.native_handle(), sz.width, sz.height)} {
    if (!ptr_)
      throw std::runtime_error{"Failed to create EGL window for surface"};
  }

  size get_attached_size() {
    size sz;
    wl_egl_window_get_attached_size(ptr_.get(), &sz.width, &sz.height);
    return sz;
  }

  void resize(size sz, point vec) {
    wl_egl_window_resize(ptr_.get(), sz.width, sz.height, vec.x, vec.y);
  }

  explicit operator bool () const noexcept {return ptr_ != nullptr;}
  native_handle_type native_handle() const noexcept {return ptr_.get();}

private:
  unique_ptr<wl_egl_window> ptr_;
};

}
}
