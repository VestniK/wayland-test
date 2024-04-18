#include <spdlog/spdlog.h>

#include <util/clock.hpp>
#include <util/task_guard.hpp>

#include <wayland/egl.hpp>
#include <wayland/event_loop.hpp>
#include <wayland/gles_window.hpp>
#include <wayland/gui_shell.hpp>
#include <wayland/ui_category.hpp>
#include <wayland/xdg_window.hpp>

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
  if (eglChooseConfig(egl_display.native_handle(), cfg_attr, &cfg, 1, &count) ==
      EGL_FALSE)
    throw std::system_error{eglGetError(), egl::category(), "eglChooseConfig"};

  eglSwapInterval(egl_display.native_handle(), 0);
  return egl::context{std::move(egl_display), cfg};
}

constexpr uint32_t ivi_main_glapp_id = 1337;

} // namespace

gles_context::gles_context(wl_display& display, wl_surface& surf, size sz)
    : egl_surface_(make_egl_context(display)),
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

struct gles_window::impl : public xdg::delegate {
  impl(co::pool_executor exec, event_queue queue, queues_notify_callback cb,
      wl_surface& surf, size initial_size, render_function render_func)
      : render_task_guard{exec,
            [&surf, &resize_channel = resize_channel, initial_size,
                queue = std::move(queue), render_func = std::move(render_func)](
                std::stop_token stop) mutable {
              gles_context ctx{queue.display(), surf, initial_size};
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

gles_window::gles_window(event_loop& eloop, co::pool_executor pool_exec,
    wl::sized_window<wl::shell_window>&& wnd, render_function render_func)
    : wnd_{std::move(wnd.window)} {
  event_queue queue = eloop.make_queue();
  wl_surface& surf = wnd_.get_surface();
  wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(&surf), &queue.get());
  auto cb = eloop.notify_queues_callback();
  impl_ = std::make_unique<impl>(
      pool_exec, std::move(queue), cb, surf, wnd.sz, std::move(render_func));
  wnd_.set_delegate(impl_.get());
}

gles_window::gles_window(gles_window&&) noexcept = default;
gles_window& gles_window::operator=(gles_window&&) noexcept = default;

gles_window::~gles_window() noexcept = default;

[[nodiscard]] bool gles_window::is_closed() const noexcept {
  return !impl_ || impl_->render_task_guard.is_finished() ||
         impl_->render_task_guard.is_finishing();
}
