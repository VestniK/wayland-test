#include <spdlog/spdlog.h>

#include <libs/sync/task_guard.hpp>

#include <libs/anime/clock.hpp>
#include <libs/eglctx/egl.hpp>

#include <libs/wlwnd/animation_window.hpp>
#include <libs/wlwnd/event_loop.hpp>
#include <libs/wlwnd/gui_shell.hpp>
#include <libs/wlwnd/ui_category.hpp>
#include <libs/wlwnd/vsync_frames.hpp>
#include <libs/wlwnd/xdg_window.hpp>

struct animation_window::impl : public xdg::delegate {
  impl(
      co::pool_executor exec, event_queue& queue, wl_surface& surf, size initial_size,
      animation_function render_func
  )
      : resize_channel{initial_size},
        render_task_guard{
            exec,
            [&surf, &resize_channel = resize_channel, &queue,
             render_func = std::move(render_func)](std::stop_token stop) mutable {
              std::stop_callback wake_queue_on_stop{stop, [&queue] { queue.wake(); }};
              vsync_frames frames{queue, surf, stop};
              render_func(queue.display(), surf, frames, resize_channel);
            }
        } {}

  void resize(size sz) override { resize_channel.update(sz); }
  void close() override {
    spdlog::debug("Window close event received");
    render_task_guard.stop();
  }

  value_update_channel<size> resize_channel;
  task_guard render_task_guard;
};

animation_window::animation_window(
    event_queue queue, co::pool_executor pool_exec, wl::sized_window<wl::shell_window>&& wnd,
    animation_function render_func
)
    : wnd_{std::move(wnd.window)}, queue_{std::move(queue)} {
  wl_surface& surf = wnd_.get_surface();
  wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(&surf), &queue_.get());
  impl_ = std::make_unique<impl>(pool_exec, queue_, surf, wnd.sz, std::move(render_func));
  wnd_.set_delegate(impl_.get());
}

animation_window::animation_window(animation_window&&) noexcept = default;
animation_window& animation_window::operator=(animation_window&&) noexcept = default;

animation_window::~animation_window() noexcept = default;

[[nodiscard]] bool animation_window::is_closed() const noexcept {
  return !impl_ || impl_->render_task_guard.is_finished() || impl_->render_task_guard.is_finishing();
}
