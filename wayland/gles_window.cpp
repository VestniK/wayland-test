#include <numeric>

#include <wayland/egl.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/ui_category.hpp>

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

  return egl::context{std::move(egl_display), cfg};
}

} // namespace

gles_window::gles_window(event_loop& eloop)
    : toplevel_window(*eloop.get_compositor(), *eloop.get_wm()), eloop_{eloop},
      egl_surface_(make_egl_context(eloop.get_display())) {}

gles_window::~gles_window() noexcept {
  if (egl_surface_)
    egl_surface_.release_thread();
}

bool gles_window::is_initialized() const noexcept { return renderer_.has_value(); }

bool gles_window::paint() {
  if (!renderer_)
    return false;

  renderer_->draw(std::chrono::steady_clock::now());
  egl_surface_.swap_buffers();
  return true;
}

size gles_window::get_size() const noexcept {
  size sz;
  wl_egl_window_get_attached_size(egl_wnd_.get(), &sz.width, &sz.height);
  return sz;
}

void gles_window::resize(size sz) {
  if (egl_wnd_ && get_size() == sz)
    return;
  if (!egl_wnd_) {
    egl_wnd_ = wl::unique_ptr<wl_egl_window>{wl_egl_window_create(get_surface(), sz.width, sz.height)};
    egl_surface_.set_window(*egl_wnd_);
  } else
    wl_egl_window_resize(egl_wnd_.get(), sz.width, sz.height, 0, 0);

  egl_surface_.make_current();
  if (!renderer_)
    renderer_.emplace();
  renderer_->resize(sz);
}

void gles_window::display(wl_output& disp [[maybe_unused]]) {}

std::error_code gles_window::draw_frame() {
  if (is_closed())
    return ui_errc::window_closed;
  paint();
  std::error_code ec;
  eloop_.dispatch_pending(ec);
  return ec;
}

std::error_code gles_window::draw_for(std::chrono::milliseconds duration) {
  auto now = std::chrono::steady_clock::now();
  const auto end = now + duration;
  std::error_code ec;
  for (; now < end; now = std::chrono::steady_clock::now()) {
    if (is_closed())
      return ui_errc::window_closed;
    paint();
    eloop_.dispatch_pending(ec);
    if (ec)
      return ec;
  }
  return ec;
}
