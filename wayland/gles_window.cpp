#include <numeric>
#include <optional>

#include <spdlog/spdlog.h>

#include <wayland/egl.hpp>
#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/ui_category.hpp>
#include <wayland/util/channel.hpp>
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

namespace {

constexpr uint32_t ivi_main_glapp_id = 1337;

} // namespace

struct gles_window::impl : public xdg::delegate {
  impl(asio::static_thread_pool::executor_type exec, event_loop &eloop,
       wl_surface &surf, size initial_size,
       wl::unique_ptr<wl_event_queue> queue)
      : render_task_guard{
            exec,
            [&eloop, &surf, &resize_channel = resize_channel, initial_size,
             queue = std::move(queue)](std::stop_token stop) mutable {
              gles_context ctx{eloop, surf, initial_size};
              spdlog::debug("OpenGL ES2 context created");

              renderer render;
              render.resize(initial_size);

              spdlog::debug("Renderer initialized. Starting render loop");
              while (!stop.stop_requested()) {

                wl::unique_ptr<wl_callback> frame_cb{wl_surface_frame(&surf)};

                if (const auto sz = resize_channel.get_update())
                  render.resize(sz.value());

                render.draw(std::chrono::steady_clock::now());
                ctx.egl_surface().swap_buffers();

                std::optional<uint32_t> next_frame;
                wl_callback_listener listener = {
                    .done = [](void *data, wl_callback *, uint32_t ts) {
                      *reinterpret_cast<std::optional<uint32_t> *>(data) = ts;
                    }};
                wl_callback_add_listener(frame_cb.get(), &listener,
                                         &next_frame);
                while (!next_frame && !stop.stop_requested())
                  wl_display_dispatch_queue(&eloop.get_display(), queue.get());
              }
              spdlog::debug("rendering finished");
            }} {}

  void resize(size sz) override { resize_channel.update(sz); }
  void close() override {
    spdlog::debug("Window close event received");
    render_task_guard.stop();
  }

  value_update_channel<size> resize_channel;
  task_guard render_task_guard;
};

asio::awaitable<gles_window>
gles_window::create(event_loop &eloop, asio::io_context::executor_type io_exec,
                    asio::thread_pool::executor_type pool_exec) {
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
      : gles_window{eloop, pool_exec, std::move(wnd), *szdelegate.wnd_size};
}

gles_window::gles_window(event_loop &eloop,
                         asio::thread_pool::executor_type pool_exec,
                         any_window wnd, size initial_size)
    : wnd_{std::move(wnd)} {
  wl::unique_ptr<wl_event_queue> queue{
      wl_display_create_queue(&eloop.get_display())};
  wl_surface &surf = std::visit(
      [](auto &wnd) -> wl_surface & { return wnd.get_surface(); }, wnd_);
  wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(&surf), queue.get());
  impl_ = std::make_unique<impl>(pool_exec, eloop, surf, initial_size,
                                 std::move(queue));
  std::visit([this](auto &wnd) { wnd.set_delegate(impl_.get()); }, wnd_);
}

gles_window::gles_window(gles_window &&) noexcept = default;
gles_window &gles_window::operator=(gles_window &&) noexcept = default;

gles_window::~gles_window() noexcept = default;

[[nodiscard]] bool gles_window::is_closed() const noexcept {
  return !impl_ || impl_->render_task_guard.is_finished() ||
         impl_->render_task_guard.is_finishing();
}
