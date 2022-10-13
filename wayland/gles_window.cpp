#include <numeric>
#include <optional>

#include <wayland/egl.hpp>
#include <wayland/event_loop.hpp>
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

  eglSwapInterval(egl_display.native_handle(), 0);
  return egl::context{std::move(egl_display), cfg};
}

} // namespace

gles_context::gles_context(const event_loop &eloop, wl_surface &surf, size sz)
    : egl_surface_(make_egl_context(eloop.get_display())),
      egl_wnd_{wl_egl_window_create(&surf, sz.width, sz.height)} {
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

gles_delegate::gles_delegate(const event_loop &eloop, wl_surface &surf, size sz)
    : ctx_(eloop, surf, sz) {
  renderer_.resize(sz);
}

bool gles_delegate::paint() {
  renderer_.draw(std::chrono::steady_clock::now());
  ctx_.egl_surface().swap_buffers();
  return true;
}

void gles_delegate::resize(size sz) {
  if (ctx_.resize(sz))
    renderer_.resize(sz);
}

namespace {

constexpr uint32_t ivi_main_glapp_id = 1337;

} // namespace

asio::awaitable<gles_window>
gles_window::create(event_loop &eloop, asio::io_context::executor_type exec) {
  struct : xdg::delegate {
    void resize(size sz) override { wnd_size = sz; }
    void close() override { closed = true; }

    std::optional<size> wnd_size;
    bool closed = false;
  } szdelegate;

  any_window wnd =
      eloop.get_ivi()
          ? any_window{std::in_place_type<ivi::window>, *eloop.get_compositor(),
                       *eloop.get_ivi(), ivi_main_glapp_id, &szdelegate}
          : any_window{std::in_place_type<xdg::toplevel_window>,
                       *eloop.get_compositor(), *eloop.get_xdg_wm(),
                       &szdelegate};
  if (auto *xdg_wnd = std::get_if<xdg::toplevel_window>(&wnd))
    xdg_wnd->maximize();
  while (!szdelegate.wnd_size && !szdelegate.closed)
    co_await eloop.dispatch(exec);

  co_return szdelegate.closed
      ? gles_window{}
      : gles_window{eloop, std::move(wnd), *szdelegate.wnd_size};
}

gles_window::gles_window(event_loop &eloop, any_window wnd, size initial_size)
    : closed_{std::make_unique<std::atomic_flag>()} {
  wl::unique_ptr<wl_event_queue> queue{
      wl_display_create_queue(&eloop.get_display())};
  wl_surface &surf = std::visit(
      [](auto &wnd) -> wl_surface & { return wnd.get_surface(); }, wnd);
  std::visit([&](auto &wnd) { wnd.set_delegate_queue(*queue); }, wnd);
  wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(&surf), queue.get());
  render_thread_ = std::jthread{
      [&eloop, &surf, initial_size, &closed = *closed_, wnd = std::move(wnd),
       queue = std::move(queue)](std::stop_token stop) mutable {
        gles_delegate gl_delegate{eloop, surf, initial_size};
        std::visit([&](auto &wnd) { wnd.set_delegate(&gl_delegate); }, wnd);

        while (!gl_delegate.is_closed() && !stop.stop_requested()) {
          wl::unique_ptr<wl_callback> frame_cb{wl_surface_frame(&surf)};
          gl_delegate.paint();
          std::optional<uint32_t> next_frame;
          wl_callback_listener listener = {
              .done = [](void *data, wl_callback *, uint32_t ts) {
                *reinterpret_cast<std::optional<uint32_t> *>(data) = ts;
              }};
          wl_callback_add_listener(frame_cb.get(), &listener, &next_frame);
          while (!next_frame && !gl_delegate.is_closed() &&
                 !stop.stop_requested())
            wl_display_dispatch_queue(&eloop.get_display(), queue.get());
        }
        closed.test_and_set();
      }};
}
