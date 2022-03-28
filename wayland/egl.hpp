#pragma once

#include <wayland-egl.h>

#include <EGL/egl.h>

#include <system_error>
#include <utility>

#if defined(minor)
#undef minor
#endif

#if defined(major)
#undef major
#endif

namespace egl {

const std::error_category &category();

class display {
public:
  display() noexcept = default;
  explicit display(wl_display &native_display)
      : disp_(eglGetDisplay(&native_display)) {
    if (disp_ == EGL_NO_DISPLAY)
      throw std::runtime_error{"Failed to get EGL display"};
    if (eglInitialize(disp_, nullptr, nullptr) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglInitialize"};
  }

  display(const display &) = delete;
  display &operator=(const display &) = delete;

  display(display &&rhs) noexcept
      : disp_{std::exchange(rhs.disp_, EGL_NO_DISPLAY)} {}
  display &operator=(display &&rhs) noexcept {
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
  native_handle_type native_handle() const noexcept { return disp_; }

  explicit operator bool() const noexcept { return disp_ != EGL_NO_DISPLAY; }

  void bind_api(EGLenum api) {
    if (eglBindAPI(api) != EGL_TRUE)
      throw std::system_error{eglGetError(), category(), "eglBindAPI"};
  }

private:
  EGLDisplay disp_ = EGL_NO_DISPLAY;
};

class context {
public:
  context() noexcept = default;
  explicit context(display disp, EGLConfig cfg)
      : display_{std::move(disp)}, cfg_{cfg} {
    const EGLint attr[] = {EGL_CONTEXT_MAJOR_VERSION, 2,
                           EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};
    ctx_ =
        eglCreateContext(display_.native_handle(), cfg_, EGL_NO_CONTEXT, attr);
    if (ctx_ == EGL_NO_CONTEXT)
      throw std::system_error{eglGetError(), category(), "eglCreateContext"};
  }
  ~context() {
    if (ctx_ != EGL_NO_CONTEXT)
      eglDestroyContext(display_.native_handle(), ctx_);
  }

  context(const context &) = delete;
  context &operator=(const context &) = delete;
  context(context &&rhs) noexcept
      : display_{std::move(rhs.display_)},
        cfg_(rhs.cfg_), ctx_{std::exchange(rhs.ctx_, EGL_NO_CONTEXT)} {}
  context &operator=(context &&rhs) noexcept {
    if (ctx_ != EGL_NO_CONTEXT)
      eglDestroyContext(display_.native_handle(), ctx_);
    display_ = std::move(rhs.display_);
    cfg_ = rhs.cfg_;
    ctx_ = std::exchange(rhs.ctx_, EGL_NO_CONTEXT);
    return *this;
  }

  using native_handle_type = EGLContext;
  native_handle_type native_handle() const noexcept { return ctx_; }

  EGLDisplay get_display() const noexcept { return display_.native_handle(); }

  EGLConfig get_config() const noexcept { return cfg_; }

private:
  display display_;
  EGLConfig cfg_;
  EGLContext ctx_ = EGL_NO_CONTEXT;
};

class surface {
public:
  surface() noexcept = default;
  surface(const surface &) = delete;
  surface &operator=(const display &) = delete;

  surface(context ctx) : ctx_{std::move(ctx)} {}
  ~surface() {
    if (surf_ != EGL_NO_SURFACE)
      eglDestroySurface(ctx_.get_display(), surf_);
  }

  surface(surface &&rhs) noexcept
      : ctx_(std::move(rhs.ctx_)),
        surf_(std::exchange(rhs.surf_, EGL_NO_SURFACE)) {}

  surface &operator=(surface &&rhs) noexcept {
    if (surf_ != EGL_NO_SURFACE)
      eglDestroySurface(ctx_.get_display(), surf_);
    ctx_ = std::move(rhs.ctx_);
    surf_ = std::exchange(rhs.surf_, EGL_NO_SURFACE);
    return *this;
  }

  using native_handle_type = EGLSurface;
  native_handle_type native_handle() const noexcept { return surf_; }

  operator bool() const noexcept { return surf_ != EGL_NO_SURFACE; }

  void set_window(wl_egl_window &wnd) {
    if (surf_ != EGL_NO_SURFACE)
      eglDestroySurface(ctx_.get_display(), surf_);
    surf_ = eglCreateWindowSurface(ctx_.get_display(), ctx_.get_config(), &wnd,
                                   nullptr);
    if (surf_ == EGL_NO_SURFACE)
      throw std::system_error{eglGetError(), category(),
                              "eglCreateWindowSurface"};
  }

  void make_current() {
    if (eglMakeCurrent(ctx_.get_display(), surf_, surf_,
                       ctx_.native_handle()) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglMakeCurrent"};
  }

  void release_thread() {
    if (eglReleaseThread() == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglReleaseThread"};
  }

  void swap_buffers() {
    if (eglSwapBuffers(ctx_.get_display(), surf_) == EGL_FALSE)
      throw std::system_error{eglGetError(), category(), "eglSwapBuffers"};
  }

private:
  context ctx_;
  EGLSurface surf_ = EGL_NO_SURFACE;
};

} // namespace egl
