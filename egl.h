#pragma once

#include <wayland-egl.h>

#include <EGL/egl.h>

#include <system_error>

#include <gsl/pointers>

#if defined(minor)
#undef minor
#endif

#if defined(major)
#undef major
#endif

namespace egl {

const std::error_category& category();

class surface {
public:
  surface() = default;

  surface(EGLDisplay disp, EGLConfig cfg, gsl::not_null<wl_egl_window*> wnd) {
    reset(disp, cfg, wnd);
  }
  ~surface() {
    if (surf_ != EGL_NO_SURFACE)
      eglDestroySurface(disp_, surf_);
  }

  void reset(EGLDisplay disp, EGLConfig cfg, gsl::not_null<wl_egl_window*> wnd) {
    if (surf_ != EGL_NO_SURFACE)
      eglDestroySurface(disp_, surf_);
    surf_ = eglCreateWindowSurface(disp, cfg, wnd, nullptr);
    if (surf_ == EGL_NO_SURFACE)
      throw std::system_error{eglGetError(), category(), "eglCreateWindowSurface"};
    disp_ = disp;
  }

  surface(const surface&) = delete;
  surface& operator= (const surface&) = delete;
  surface(surface&&) = delete;
  surface& operator= (surface&&) = delete;

  using native_handle_type = EGLSurface;
  native_handle_type native_handle() const noexcept {return surf_;}

  void swap_buffers() {
    if (eglSwapBuffers(disp_, surf_) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglSwapBuffers"};
  }

private:
  EGLDisplay disp_ = EGL_NO_DISPLAY;
  EGLSurface surf_ = EGL_NO_SURFACE;
};

class display {
public:
  display() noexcept = default;
  explicit display(wl_display& native_display): disp_(eglGetDisplay(&native_display)) {
    if (disp_ == EGL_NO_DISPLAY)
      throw std::runtime_error{"Failed to get EGL display"};
    if (eglInitialize(disp_, nullptr, nullptr) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglInitialize"};
  }

  display(const display&) = delete;
  display& operator= (const display&) = delete;

  display(display&& rhs) noexcept: disp_{std::exchange(rhs.disp_, EGL_NO_DISPLAY)} {}
  display& operator= (display&& rhs) noexcept {
    if (disp_ != EGL_NO_DISPLAY)
      eglTerminate(disp_);
    disp_ = std::exchange(rhs.disp_, EGL_NO_DISPLAY);
    return *this;
  }

  ~display() noexcept {
    if (disp_ != EGL_NO_DISPLAY)
      eglTerminate(disp_);
  }

  using native_handle_type = EGLDisplay;
  native_handle_type native_handle() const noexcept {return disp_;}

  explicit operator bool () const noexcept {return disp_ != EGL_NO_DISPLAY;}

  void bind_api(EGLenum api) {
    if (eglBindAPI(api) != EGL_TRUE)
      throw std::system_error{eglGetError(), category(), "eglBindAPI"};
  }

  surface create_surface(EGLConfig cfg, gsl::not_null<wl_egl_window*> wnd) {
    return surface{disp_, cfg, wnd};
  }

private:
  EGLDisplay disp_ = EGL_NO_DISPLAY;
};

class context: private display {
public:
  explicit context(display disp, EGLConfig cfg): display(std::move(disp)), cfg_(cfg) {
    const EGLint attr[] = {
      EGL_CONTEXT_MAJOR_VERSION, 2,
      EGL_CONTEXT_MINOR_VERSION, 0,
      EGL_NONE
    };
    ctx_ = eglCreateContext(display::native_handle(), cfg_, EGL_NO_CONTEXT, attr);
    if (ctx_ == EGL_NO_CONTEXT)
      throw std::system_error{eglGetError(), category(), "eglCreateContext"};
  }
  ~context() {
    eglDestroyContext(display::native_handle(), ctx_);
  }

  context(const context&) = delete;
  context& operator= (const context&) = delete;
  context(context&&) = delete;
  context& operator= (context&&) = delete;

  using native_handle_type = EGLContext;
  native_handle_type native_handle() const noexcept {return ctx_;}

  EGLConfig get_config() const noexcept {return cfg_;}

  void make_current(const surface& surf) {
    if (eglMakeCurrent(display::native_handle(), surf.native_handle(), surf.native_handle(), ctx_) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglMakeCurrent"};
  }

  void reset_surface(surface& surf, gsl::not_null<wl_egl_window*> wnd) {
    surf.reset(display::native_handle(), cfg_, wnd);
  }

private:
  EGLConfig cfg_;
  EGLContext ctx_;
};

} // namespace egl
