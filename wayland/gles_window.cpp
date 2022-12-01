#include <iterator>
#include <numeric>
#include <optional>

#include <spdlog/spdlog.h>

#include <wayland/egl.hpp>
#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/ui_category.hpp>
#include <wayland/util/clock.hpp>
#include <wayland/util/task_guard.hpp>

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

constexpr uint32_t ivi_main_glapp_id = 1337;

} // namespace

gles_context::gles_context(wl_display &display, wl_surface &surf, size sz)
    : egl_surface_(make_egl_context(display)), egl_wnd_{wl_egl_window_create(
                                                   &surf, sz.width,
                                                   sz.height)} {
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

struct gles_window::impl : public xdg::delegate {
  impl(asio::static_thread_pool::executor_type exec, event_queue queue,
       queues_notify_callback cb, wl_surface &surf, size initial_size,
       render_function render_func)
      : render_task_guard{exec,
                          [&surf, &resize_channel = resize_channel,
                           initial_size, queue = std::move(queue),
                           render_func = std::move(render_func)](
                              std::stop_token stop) mutable {
                            gles_context ctx{queue.display(), surf,
                                             initial_size};
                            spdlog::debug("OpenGL ES2 context created");

                            vsync_frames frames{queue, surf, stop};
                            ctx.egl_surface().swap_buffers();

                            render_func(ctx, frames, resize_channel);
                            spdlog::debug("rendering finished");
                          }},
        on_stop{render_task_guard.stop_token(), cb} {}

  void resize(size sz) override { resize_channel.update(sz); }
  void close() override {
    spdlog::debug("Window close event received");
    render_task_guard.stop();
  }

  value_update_channel<size> resize_channel;
  task_guard render_task_guard;
  std::stop_callback<queues_notify_callback> on_stop;
};

asio::awaitable<gles_window> gles_window::create_maximized(
    event_loop &eloop, asio::io_context::executor_type io_exec,
    asio::thread_pool::executor_type pool_exec, render_function render_func) {
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
    co_await eloop.dispatch_once(io_exec);

  co_return szdelegate.closed
      ? gles_window{}
      : gles_window{eloop, pool_exec, std::move(wnd), *szdelegate.wnd_size,
                    std::move(render_func)};
}

gles_window::gles_window(event_loop &eloop,
                         asio::thread_pool::executor_type pool_exec,
                         any_window wnd, size initial_size,
                         render_function render_func)
    : wnd_{std::move(wnd)} {
  event_queue queue = eloop.make_queue();
  wl_surface &surf = std::visit(
      [](auto &wnd) -> wl_surface & { return wnd.get_surface(); }, wnd_);
  wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(&surf), &queue.get());
  auto cb = queue.notify_callback();
  impl_ = std::make_unique<impl>(pool_exec, std::move(queue), cb, surf,
                                 initial_size, std::move(render_func));
  std::visit([this](auto &wnd) { wnd.set_delegate(impl_.get()); }, wnd_);
}

gles_window::gles_window(gles_window &&) noexcept = default;
gles_window &gles_window::operator=(gles_window &&) noexcept = default;

gles_window::~gles_window() noexcept = default;

[[nodiscard]] bool gles_window::is_closed() const noexcept {
  return !impl_ || impl_->render_task_guard.is_finished() ||
         impl_->render_task_guard.is_finishing();
}
