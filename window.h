#pragma once

#include "geom.h"
#include "wlutil.hpp"

struct wl_compositor;
struct wl_surface;
struct xdg_wm_base;

class window {
public:
  struct delegate {
    virtual ~delegate();
    /// @pre wnd.valid()
    virtual void resize(window& wnd, size sz) = 0;
    virtual void close(window& wnd) = 0;
  };

public:
  window() noexcept = default;

  window(const window&) = delete;
  window& operator= (const window&) = delete;

  window(window&& rhs) noexcept;

  window& operator= (window&& rhs) noexcept;

  ~window() = default;

  static window create_toplevel(wl_compositor& compositor, xdg_wm_base& shell);

  void set_delegate(delegate* val) {delegate_ = val;}

  bool valid() const noexcept {return surface_ && toplevel_;}

  wl_surface* get_surface() {return surface_.get();}

  /// @pre this->valid()
  void maximize() {
    xdg_toplevel_set_maximized(toplevel_.get());
  }

private:
  window(wl::unique_ptr<wl_surface> surface,
    wl::unique_ptr<xdg_surface> xdg_surf,
    wl::unique_ptr<xdg_toplevel> toplevel
  ) noexcept:
    surface_{std::move(surface)},
    xdg_surface_{std::move(xdg_surf)},
    toplevel_{std::move(toplevel)}
  {
    xdg_toplevel_add_listener(toplevel_.get(), &toplevel_listener, this);
    xdg_surface_add_listener(xdg_surface_.get(), &surface_listener, this);
  }

  static void configure_surface(void*, xdg_surface*, uint32_t) {}
  static void configure_toplevel(void* data, xdg_toplevel*, int32_t width, int32_t height, wl_array*) {
    auto* self = reinterpret_cast<window*>(data);
    if (self->delegate_)
      self->delegate_->resize(*self, {width, height});
  }
  static void close(void* data, xdg_toplevel*) {
    auto* self = reinterpret_cast<window*>(data);
    if (self->delegate_)
      self->delegate_->close(*self);
  }

  static xdg_toplevel_listener toplevel_listener;
  static xdg_surface_listener surface_listener;

private:
  wl::unique_ptr<wl_surface> surface_;
  wl::unique_ptr<xdg_surface> xdg_surface_;
  wl::unique_ptr<xdg_toplevel> toplevel_;
  delegate* delegate_ = nullptr;
};
