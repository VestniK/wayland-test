#include "gles_context.hpp"

#include <libs/eglctx/egl.hpp>

namespace {

egl::context make_egl_context(wl_display& display) {
  egl::display egl_display{display};
  egl_display.bind_api(EGL_OPENGL_ES_API);
  EGLint count;
  // clang-format off
  const EGLint cfg_attr[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 24,
      EGL_STENCIL_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_NONE};
  // clang-format on
  EGLConfig cfg;
  if (eglChooseConfig(egl_display.native_handle(), cfg_attr, &cfg, 1, &count) == EGL_FALSE)
    throw std::system_error{eglGetError(), egl::category(), "eglChooseConfig"};

  eglSwapInterval(egl_display.native_handle(), 0);
  return egl::context{std::move(egl_display), cfg};
}

} // namespace

gles_context::gles_context(wl_display& display, wl_surface& surf, size sz)
    : egl_surface_(make_egl_context(display)), egl_wnd_{wl_egl_window_create(&surf, sz.width, sz.height)} {
  egl_surface_.set_window(*egl_wnd_);
  egl_surface_.make_current();
}

gles_context::~gles_context() noexcept { egl_surface_.release_thread(); }

[[nodiscard]] size gles_context::get_size() const noexcept {
  size sz;
  wl_egl_window_get_attached_size(egl_wnd_.get(), &sz.width, &sz.height);
  return sz;
}

bool gles_context::resize(size sz) {
  if (egl_wnd_ && get_size() == sz)
    return false;

  wl_egl_window_resize(egl_wnd_.get(), sz.width, sz.height, 0, 0);
  egl_surface_.make_current();
  return true;
}
