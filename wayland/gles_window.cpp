#include <numeric>

#include <wayland/egl.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/ui_category.hpp>

namespace {

egl::context make_egl_context(wl_display &display) {
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
  if (eglChooseConfig(egl_display.native_handle(), cfg_attr, &cfg, 1, &count) ==
      EGL_FALSE)
    throw std::system_error{eglGetError(), egl::category(), "eglChooseConfig"};

  return egl::context{std::move(egl_display), cfg};
}

} // namespace

gles_delegate::gles_delegate(event_loop &eloop, wl_surface &surf, size sz)
    : eloop_{eloop}, egl_surface_(make_egl_context(eloop.get_display())),
      egl_wnd_{wl_egl_window_create(&surf, sz.width, sz.height)} {
  egl_surface_.set_window(*egl_wnd_);
  egl_surface_.make_current();
  renderer_.emplace();
  renderer_->resize(sz);
}

gles_delegate::~gles_delegate() noexcept {
  if (egl_surface_)
    egl_surface_.release_thread();
}

bool gles_delegate::paint() {
  renderer_->draw(std::chrono::steady_clock::now());
  egl_surface_.swap_buffers();
  return true;
}

size gles_delegate::get_size() const noexcept {
  size sz;
  wl_egl_window_get_attached_size(egl_wnd_.get(), &sz.width, &sz.height);
  return sz;
}

void gles_delegate::resize(size sz) {
  if (egl_wnd_ && get_size() == sz)
    return;

  wl_egl_window_resize(egl_wnd_.get(), sz.width, sz.height, 0, 0);
  egl_surface_.make_current();
  renderer_->resize(sz);
}

std::error_code gles_delegate::draw_frame() {
  paint();
  std::error_code ec;
  eloop_.dispatch_pending(ec);
  return ec;
}

std::error_code gles_delegate::draw_for(std::chrono::milliseconds duration) {
  auto now = std::chrono::steady_clock::now();
  const auto end = now + duration;
  std::error_code ec;
  for (; now < end; now = std::chrono::steady_clock::now()) {
    paint();
    eloop_.dispatch_pending(ec);
    if (ec)
      return ec;
  }
  return ec;
}

void gles_delegate::camera_look_at(float ex, float ey, float ez, float cx,
                                 float cy, float cz) {
  renderer_->camera_look_at({ex, ey, ez}, {cx, cy, cz});
}
